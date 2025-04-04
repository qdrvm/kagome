/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "dispute_coordinator/dispute_coordinator.hpp"
#include "network/dispute_request_observer.hpp"
#include "network/types/dispute_messages.hpp"

#include <libp2p/basic/scheduler.hpp>

#include "clock/impl/basic_waitable_timer.hpp"
#include "crypto/key_store/session_keys.hpp"
#include "crypto/sr25519_provider.hpp"
#include "dispute_coordinator/chain_scraper.hpp"
#include "dispute_coordinator/impl/batches.hpp"
#include "dispute_coordinator/impl/candidate_vote_state.hpp"
#include "dispute_coordinator/impl/errors.hpp"
#include "dispute_coordinator/impl/runtime_info.hpp"
#include "dispute_coordinator/participation/types.hpp"
#include "dispute_coordinator/spam_slots.hpp"
#include "dispute_coordinator/storage.hpp"
#include "dispute_coordinator/types.hpp"
#include "injector/lazy.hpp"
#include "log/logger.hpp"
#include "metrics/metrics.hpp"
#include "network/peer_view.hpp"
#include "network/reputation_change.hpp"
#include "network/reputation_repository.hpp"
#include "parachain/types.hpp"
#include "primitives/authority_discovery_id.hpp"

namespace kagome {
  class PoolHandler;
  class PoolHandlerReady;
}  // namespace kagome

namespace kagome::application {
  class AppStateManager;
  class ChainSpec;
}  // namespace kagome::application

namespace kagome::authority_discovery {
  class Query;
}

namespace kagome::blockchain {
  class BlockTree;
}  // namespace kagome::blockchain

namespace kagome::common {
  class MainThreadPool;
}

namespace kagome::consensus {
  class Timeline;
}

namespace kagome::dispute {
  class ChainScraper;
  class Participation;
  class RuntimeInfo;
  class SendingDispute;
  class DisputeThreadPool;
}  // namespace kagome::dispute

namespace kagome::network {
  class Router;
  class PeerView;
}  // namespace kagome::network

namespace kagome::parachain {
  class ApprovalDistribution;
  class Recovery;
  class Pvf;
}  // namespace kagome::parachain

namespace kagome::runtime {
  class ParachainHost;
  class Core;
}  // namespace kagome::runtime

namespace kagome::dispute {

  class DisputeCoordinatorImpl final
      : public DisputeCoordinator,
        public network::DisputeRequestObserver,
        public std::enable_shared_from_this<DisputeCoordinatorImpl> {
   public:
    static constexpr Timestamp kActiveDurationSecs = 180;
    static constexpr size_t kPeerQueueCapacity = 10;
    // Dispute runtime version requirement
    static constexpr uint32_t kPrioritizedSelectionRuntimeVersionRequirement =
        3;

    /// Rate limit on the receiver side.
    /// The queued messages for each peer are processed
    /// every kReceiveRateLimitMs interval.
    static constexpr uint64_t kReceiveRateLimitMs = 100;

    /// It would be nice to draw this from the chain state, but we have no tools
    /// for it right now. On Polkadot this is 1 day, and on Kusama it's 6 hours.
    ///
    /// Number of sessions we want to consider in disputes.
    static constexpr auto kDisputeWindow = SessionIndex(6);

    DisputeCoordinatorImpl(
        std::shared_ptr<application::ChainSpec> chain_spec,
        std::shared_ptr<application::AppStateManager> app_state_manager,
        clock::SystemClock &system_clock,
        clock::SteadyClock &steady_clock,
        std::shared_ptr<crypto::SessionKeys> session_keys,
        std::shared_ptr<Storage> storage,
        std::shared_ptr<crypto::Sr25519Provider> sr25519_crypto_provider,
        std::shared_ptr<crypto::Hasher> hasher,
        std::shared_ptr<blockchain::BlockTree> block_tree,
        std::shared_ptr<runtime::Core> core_api,
        std::shared_ptr<runtime::ParachainHost> api,
        std::shared_ptr<parachain::Recovery> recovery,
        std::shared_ptr<parachain::Pvf> pvf,
        std::shared_ptr<parachain::ApprovalDistribution> approval_distribution,
        std::shared_ptr<authority_discovery::Query> authority_discovery,
        common::MainThreadPool &main_thread_pool,
        DisputeThreadPool &dispute_thread_pool,
        std::shared_ptr<network::Router> router,
        std::shared_ptr<network::PeerView> peer_view,
        primitives::events::ChainSubscriptionEnginePtr chain_sub_engine,
        LazySPtr<consensus::Timeline> timeline,
        std::shared_ptr<network::ReputationRepository> reputation_repository);

    bool tryStart();

    void onDisputeRequest(const libp2p::peer::PeerId &peer_id,
                          const network::DisputeMessage &request,
                          CbOutcome<void> &&cb) override;

    void onParticipation(const ParticipationStatement &message) override;

    void getRecentDisputes(CbOutcome<OutputDisputes> &&cb) override;

    void getActiveDisputes(CbOutcome<OutputDisputes> &&cb) override;

    void queryCandidateVotes(const QueryCandidateVotes &msg,
                             CbOutcome<OutputCandidateVotes> &&cb) override;

    void issueLocalStatement(SessionIndex session,
                             CandidateHash candidate_hash,
                             CandidateReceipt candidate_receipt,
                             bool valid) override;

    void determineUndisputedChain(
        primitives::BlockInfo base,
        std::vector<BlockDescription> block_descriptions,
        CbOutcome<primitives::BlockInfo> &&cb) override;

    void getDisputeForInherentData(
        const primitives::BlockInfo &relay_parent,
        std::function<void(MultiDisputeStatementSet)> &&cb) override;

   private:
    void startup(const network::ExView &updated);
    void on_active_leaves_update(const network::ExView &updated);
    void on_finalized_block(const primitives::BlockInfo &finalized);

    /// Import statements by validators about a candidate.
    ///
    /// The subsystem will silently discard ancient statements or sets of only
    /// dispute-specific statements for candidates that are previously unknown
    /// to the subsystem. The former is simply because ancient data is not
    /// relevant and the latter is as a DoS prevention mechanism. Both backing
    /// and approval statements already undergo anti-DoS procedures in their
    /// respective subsystems, but statements cast specifically for disputes are
    /// not necessarily relevant to any candidate the system is already aware of
    /// and thus present a DoS vector. Our expectation is that nodes will notify
    /// each other of disputes over the network by providing (at least) 2
    /// conflicting statements, of which one is either a backing or validation
    /// statement.
    ///
    /// This does not do any checking of the message signature.
    ///
    /// @param candidate_receipt - The candidate receipt itself
    /// @param session - The session the candidate appears in
    /// @param statements - Statements, with signatures checked, by validators
    /// participating in disputes. The validator index passed alongside each
    /// statement should correspond to the index of the validator in the set.
    /// @param cb - Callback for result
    void importStatements(
        CandidateReceipt candidate_receipt,
        SessionIndex session,
        std::vector<Indexed<SignedDisputeStatement>> statements,
        CbOutcome<void> &&cb);

    std::optional<CandidateEnvironment> makeCandidateEnvironment(
        crypto::SessionKeys &session_keys,
        SessionIndex session,
        primitives::BlockHash relay_parent,
        std::vector<ValidatorIndex> &&offchain_disabled);

    outcome::result<void> process_on_chain_votes(
        const ScrapedOnChainVotes &votes);

    outcome::result<void> process_active_leaves_update(
        const ActiveLeavesUpdate &update);

    outcome::result<void> process_finalized_block(
        const primitives::BlockInfo &finalized);

    outcome::result<bool> handle_import_statements(
        MaybeCandidateReceipt candidate_receipt,
        const SessionIndex session,
        std::vector<Indexed<SignedDisputeStatement>> statements);

    /// Create a `DisputeMessage` to be sent to `DisputeDistribution`.
    // https://github.com/paritytech/polkadot/blob/40974fb99c86f5c341105b7db53c7aa0df707d66/node/core/dispute-coordinator/src/lib.rs#L510
    outcome::result<network::DisputeMessage> make_dispute_message(
        SessionInfo info,
        CandidateVotes votes,
        SignedDisputeStatement our_vote,
        ValidatorIndex our_index);

    /// Tell dispute-distribution to send all our votes.
    ///
    /// Should be called on startup for all active disputes where there are
    /// votes from us already.
    void send_dispute_messages(const CandidateEnvironment &env,
                               const CandidateVoteState &vote_state);

    outcome::result<void> issue_local_statement(
        const CandidateHash &candidate_hash,
        const CandidateReceipt &candidate_receipt,
        SessionIndex session,
        bool valid);

    /// Determine the best block and its block number.
    /// Assumes `block_descriptions` are sorted from the one
    /// with the lowest `BlockNumber` to the highest.
    outcome::result<primitives::BlockInfo> determine_undisputed_chain(
        const primitives::BlockNumber &base_number,
        const primitives::BlockHash &base_hash,
        std::vector<BlockDescription> descriptions);

    /// Pop all heads and return them for processing.
    ///
    /// This gets one message from each peer that has sent at least one.
    void process_portion_incoming_disputes();

    /// Schedule processing next portion of requests.
    void make_task_for_next_portion();

    /// Start importing votes for the given request or batch.
    ///
    /// Signature check and in case we already have an existing batch we import
    /// to that batch, otherwise import to `dispute-coordinator` directly and
    /// open a batch.
    outcome::result<void> start_import_or_batch(
        const libp2p::peer::PeerId &peer,
        const network::DisputeMessage &request,
        CbOutcome<void> &&cb);

    void check_batches();

    void start_import(PreparedImport &&prepared_import);

    void sendDisputeResponse(outcome::result<void> res, CbOutcome<void> &&cb);

    void sendDisputeRequest(const network::DisputeMessage &request,
                            CbOutcome<void> &&cb);

    outcome::result<bool> refresh_sessions();

    void handle_active_dispute_response(
        outcome::result<OutputDisputes> active_disputes_res);

    bool has_required_runtime(const primitives::BlockInfo &relay_parent);

    log::Logger log_;
    clock::SystemClock &system_clock_;
    clock::SteadyClock &steady_clock_;
    std::shared_ptr<crypto::SessionKeys> session_keys_;
    std::shared_ptr<Storage> storage_;
    std::shared_ptr<crypto::Sr25519Provider> sr25519_crypto_provider_;
    std::shared_ptr<crypto::Hasher> hasher_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<runtime::Core> core_api_;
    std::shared_ptr<runtime::ParachainHost> api_;
    std::shared_ptr<parachain::Recovery> recovery_;
    std::shared_ptr<parachain::Pvf> pvf_;
    std::shared_ptr<parachain::ApprovalDistribution> approval_distribution_;
    std::shared_ptr<authority_discovery::Query> authority_discovery_;
    std::shared_ptr<network::Router> router_;
    std::shared_ptr<network::PeerView> peer_view_;
    primitives::events::ChainSub chain_sub_;
    LazySPtr<consensus::Timeline> timeline_;
    std::shared_ptr<network::ReputationRepository> reputation_repository_;
    std::shared_ptr<PoolHandler> main_pool_handler_;
    std::shared_ptr<PoolHandlerReady> dispute_thread_handler_;

    std::shared_ptr<network::PeerView::MyViewSubscriber> my_view_sub_;

    std::atomic_bool initialized_ = false;

    std::unique_ptr<ChainScraper> scraper_;
    SessionIndex highest_session_{0};
    std::shared_ptr<SpamSlots> spam_slots_;
    std::shared_ptr<Participation> participation_;

    /// Queues for messages from authority peers for rate limiting.
    ///
    /// Invariants ensured:
    ///
    /// 1. No queue will ever have more than `PEER_QUEUE_CAPACITY` elements.
    /// 2. There are no empty queues. Whenever a queue gets empty, it is
    ///    removed. This way checking whether there are any messages queued is
    ///    cheap.
    /// 3. As long as not empty, `pop_reqs` will, if called in sequence, not
    ///    return `Ready` more often than once for every `RECEIVE_RATE_LIMIT`,
    ///    but it will always return Ready eventually.
    /// 4. If empty `pop_reqs` will never return `Ready`, but will always be
    ///    `Pending`.
    /// Actual queues.
    std::unordered_map<
        primitives::AuthorityDiscoveryId,
        std::deque<std::tuple<network::DisputeMessage, CbOutcome<void>>>>
        queues_;

    /// Collection of DisputeRequests from disabled validators
    std::unordered_map<CandidateHash,
                       std::tuple<primitives::AuthorityDiscoveryId,
                                  network::DisputeMessage,
                                  CbOutcome<void>>>
        requests_from_disabled_validators_;

    /// Delay timer for establishing the rate limit.
    std::optional<libp2p::basic::Scheduler::Handle> rate_limit_timer_;

    std::shared_ptr<libp2p::basic::Scheduler> scheduler_;

    std::shared_ptr<RuntimeInfo> runtime_info_;

    /// Currently active batches of imports per candidate.
    std::unique_ptr<Batches> batches_;
    std::optional<libp2p::basic::Scheduler::Handle> batch_collecting_timer_;

    /// All heads we currently consider active.
    std::unordered_set<primitives::BlockHash> active_heads_;

    /// List of currently active sessions.
    ///
    /// Value is the hash that was used for the query.
    std::unordered_map<SessionIndex, primitives::BlockHash> active_sessions_;

    /// State we keep while waiting for active disputes.
    ///
    /// When we send `DisputeCoordinatorMessage::ActiveDisputes`, this is the
    /// state we keep while waiting for the response.
    struct WaitForActiveDisputesState {
      /// Have we seen any new sessions since last refresh?
      bool have_new_sessions;
    };

    std::optional<WaitForActiveDisputesState> waiting_for_active_disputes_;

    /// All ongoing dispute sendings this subsystem is aware of.
    ///
    /// Using an `IndexMap` so items can be iterated in the order of insertion.
    std::list<std::tuple<CandidateHash, std::shared_ptr<SendingDispute>>>
        sending_disputes_;

    // Ideally, we want to use the top `byzantine_threshold` offenders here
    // based on the amount of stake slashed. However, given that slashing might
    // be applied with a delay, we want to have some list of offenders as soon
    // as disputes conclude offchain. This list only approximates the top
    // offenders and only accounts for lost disputes. But that should be good
    // enough to prevent spam attacks.
    struct LostSessionDisputes {
      std::deque<ValidatorIndex> backers_for_invalid;
      std::deque<ValidatorIndex> for_invalid;
      std::deque<ValidatorIndex> against_valid;
    };

    // We separate lost disputes to prioritize "for invalid" offenders. And
    // among those, we prioritize backing votes the most. There's no need to
    // limit the size of these sets, as they are already limited by the number
    // of validators in the session. We use deque to ensure the iteration order
    // prioritizes most recently disputes lost over older ones in case we reach
    // the limit.
    struct OffchainDisabledValidators {
      std::map<SessionIndex, LostSessionDisputes> per_session;

      void prune_old(SessionIndex up_to_excluding) {
        std::erase_if(per_session,
                      [&](const auto &p) { return p.first < up_to_excluding; });
      }

      void insert_for_invalid(SessionIndex session_index,
                              ValidatorIndex validator_index,
                              bool is_backer) {
        auto &entry = per_session[session_index];
        if (is_backer) {
          entry.backers_for_invalid.emplace_front(validator_index);
        } else {
          entry.for_invalid.emplace_front(validator_index);
        }
      }

      void insert_against_valid(SessionIndex session_index,
                                ValidatorIndex validator_index) {
        per_session[session_index].against_valid.emplace_front(validator_index);
      }

      std::vector<ValidatorIndex> asVector(SessionIndex session_index) {
        std::vector<ValidatorIndex> res;
        auto it = per_session.find(session_index);
        if (it == per_session.end()) {
          return res;
        }
        const auto &entry = it->second;

        res.reserve(entry.backers_for_invalid.size()  //
                    + entry.for_invalid.size()        //
                    + entry.against_valid.size());

        for (auto c : {std::cref(entry.backers_for_invalid),
                       std::cref(entry.for_invalid),
                       std::cref(entry.against_valid)}) {
          res.insert(res.end(), c.get().begin(), c.get().end());
        }
        return res;
      }
    };

    OffchainDisabledValidators offchain_disabled_validators_;

    // Metrics
    metrics::RegistryPtr metrics_registry_ = metrics::createRegistry();
    metrics::Counter *metric_disputes_total_;
    metrics::Counter *metric_votes_valid_;
    metrics::Counter *metric_votes_invalid_;
    metrics::Counter *metric_concluded_valid_;
    metrics::Counter *metric_concluded_invalid_;
    metrics::Gauge *metric_disputes_finality_lag_;
  };

}  // namespace kagome::dispute
