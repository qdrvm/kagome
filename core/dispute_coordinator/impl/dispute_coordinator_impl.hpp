/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_DISPUTE_DISPUTECOORDINATORIMPL
#define KAGOME_DISPUTE_DISPUTECOORDINATORIMPL

#include "dispute_coordinator/dispute_coordinator.hpp"
#include "network/dispute_request_observer.hpp"

#include <boost/asio/io_context.hpp>
#include <list>

#include "clock/impl/basic_waitable_timer.hpp"
#include "crypto/crypto_store/session_keys.hpp"
#include "crypto/sr25519_provider.hpp"
#include "dispute_coordinator/chain_scraper.hpp"
#include "dispute_coordinator/impl/batches.hpp"
#include "dispute_coordinator/impl/candidate_vote_state.hpp"
#include "dispute_coordinator/impl/errors.hpp"
#include "dispute_coordinator/impl/runtime_info.hpp"
#include "dispute_coordinator/participation/types.hpp"
#include "dispute_coordinator/rolling_session_window.hpp"
#include "dispute_coordinator/spam_slots.hpp"
#include "dispute_coordinator/storage.hpp"
#include "dispute_coordinator/types.hpp"
#include "log/logger.hpp"
#include "network/peer_view.hpp"
#include "parachain/types.hpp"
#include "primitives/authority_discovery_id.hpp"

namespace kagome {
  class ThreadPool;
  class ThreadHandler;
}  // namespace kagome

namespace kagome::application {
  class AppStateManager;
}

namespace kagome::authority_discovery {
  class Query;
}

namespace kagome::blockchain {
  class BlockTree;
  class BlockHeaderRepository;
}  // namespace kagome::blockchain

namespace kagome::dispute {
  class ChainScraper;
  class Participation;
  class RuntimeInfo;
  class SendingDispute;
}  // namespace kagome::dispute

namespace kagome::network {
  struct DisputeMessage;
  class Router;
  class PeerView;
}  // namespace kagome::network

namespace kagome::parachain {
  struct ApprovalDistribution;
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

    /// Rate limit on the `receiver` side.
    ///
    /// If messages from one peer come in at a higher rate than every
    /// `RECEIVE_RATE_LIMIT` on average, we start dropping messages from that
    /// peer to enforce that limit.
    static constexpr auto kReceiveRateLimit = std::chrono::milliseconds(100);

    DisputeCoordinatorImpl(
        std::shared_ptr<application::AppStateManager> app_state_manager,
        clock::SystemClock &clock,
        std::shared_ptr<crypto::SessionKeys> session_keys,
        std::shared_ptr<Storage> storage,
        std::shared_ptr<crypto::Sr25519Provider> sr25519_crypto_provider,
        std::shared_ptr<blockchain::BlockHeaderRepository>
            block_header_repository,
        std::shared_ptr<crypto::Hasher> hasher,
        std::shared_ptr<blockchain::BlockTree> block_tree,
        std::shared_ptr<runtime::Core> core_api,
        std::shared_ptr<runtime::ParachainHost> api,
        std::shared_ptr<parachain::Recovery> recovery,
        std::shared_ptr<parachain::Pvf> pvf,
        std::shared_ptr<parachain::ApprovalDistribution> approval_distribution,
        std::shared_ptr<authority_discovery::Query> authority_discovery,
        std::shared_ptr<boost::asio::io_context> main_thread_context,
        std::shared_ptr<network::Router> router,
        std::shared_ptr<network::PeerView> peer_view,
        primitives::events::BabeStateSubscriptionEnginePtr
            babe_status_observable);

    bool prepare();
    bool start();
    void stop();

    void onDisputeRequest(const libp2p::peer::PeerId &peer_id,
                          const network::DisputeMessage &request,
                          CbOutcome<void> &&cb) override;

    void handle_incoming_ImportStatements(
        CandidateReceipt candidate_receipt,
        SessionIndex session,
        std::vector<Indexed<SignedDisputeStatement>> statements,
        CbOutcome<void> &&cb) override;

    void handle_incoming_RecentDisputes(
        CbOutcome<OutputDisputes> &&cb) override;

    void handle_incoming_ActiveDisputes(
        CbOutcome<OutputDisputes> &&cb) override;

    void handle_incoming_QueryCandidateVotes(
        const QueryCandidateVotes &msg,
        CbOutcome<OutputCandidateVotes> &&cb) override;

    void handle_incoming_IssueLocalStatement(SessionIndex session,
                                             CandidateHash candidate_hash,
                                             CandidateReceipt candidate_receipt,
                                             bool valid,
                                             CbOutcome<void> &&cb) override;

    void handle_incoming_DetermineUndisputedChain(
        primitives::BlockInfo base,
        std::vector<BlockDescription> block_descriptions,
        CbOutcome<primitives::BlockInfo> &&cb) override;

    void getDisputeForInherentData(
        const primitives::BlockInfo &relay_parent,
        std::function<void(MultiDisputeStatementSet)> &&cb) override;

   private:
    void startup(const network::ExView &updated);
    void on_participation(const ParticipationStatement &message);
    void on_active_leaves_update(const network::ExView &updated);
    void on_finalized_block(const primitives::BlockInfo &finalized);

    static std::optional<CandidateEnvironment> makeCandidateEnvironment(
        crypto::SessionKeys &session_keys,
        RollingSessionWindow &rolling_session_window,
        SessionIndex session);

    outcome::result<void> process_on_chain_votes(ScrapedOnChainVotes votes);

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
        std::vector<BlockDescription> block_descriptions);

    /// Pop all heads and return them for processing.
    ///
    /// This gets one message from each peer that has sent at least one.
    void process_portion_incoming_disputes();

    /// Schedule processing next portion of requests. This function is rate
    /// limited, if called in sequence it will not return more often than every
    /// `kReceiveRateLimit`.
    void make_task_for_next_portion();

    /// Start importing votes for the given request or batch.
    ///
    /// Signature check and in case we already have an existing batch we import
    /// to that batch, otherwise import to `dispute-coordinator` directly and
    /// open a batch.
    outcome::result<void> start_import_or_batch(
        const network::DisputeMessage &request, CbOutcome<void> &&cb);

    void start_import(PreparedImport &&prepared_import);

    void sendDisputeResponse(outcome::result<void> res, CbOutcome<void> &&cb);

    void sendDisputeRequest(const network::DisputeMessage &request,
                            CbOutcome<void> &&cb);

    outcome::result<bool> refresh_sessions();

    void handle_active_dispute_response(
        outcome::result<OutputDisputes> active_disputes_res);

    bool has_required_runtime(const primitives::BlockInfo &relay_parent);

    std::shared_ptr<application::AppStateManager> app_state_manager_;
    clock::SystemClock &clock_;
    std::shared_ptr<crypto::SessionKeys> session_keys_;
    std::shared_ptr<Storage> storage_;
    std::shared_ptr<crypto::Sr25519Provider> sr25519_crypto_provider_;
    std::shared_ptr<blockchain::BlockHeaderRepository> block_header_repository_;
    std::shared_ptr<crypto::Hasher> hasher_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<runtime::Core> core_api_;
    std::shared_ptr<runtime::ParachainHost> api_;
    std::shared_ptr<parachain::Recovery> recovery_;
    std::shared_ptr<parachain::Pvf> pvf_;
    std::shared_ptr<parachain::ApprovalDistribution> approval_distribution_;
    std::shared_ptr<authority_discovery::Query> authority_discovery_;
    std::unique_ptr<ThreadHandler> main_thread_context_;
    std::shared_ptr<network::Router> router_;
    std::shared_ptr<network::PeerView> peer_view_;
    primitives::events::BabeStateSubscriptionEnginePtr babe_status_observable_;

    std::shared_ptr<primitives::events::BabeStateEventSubscriber>
        babe_status_sub_;
    std::shared_ptr<network::PeerView::MyViewSubscriber> my_view_sub_;
    std::shared_ptr<primitives::events::ChainEventSubscriber> chain_sub_;

    std::atomic_bool was_synchronized_ = false;
    std::atomic_bool initialized_ = false;

    std::unique_ptr<ChainScraper> scraper_;
    SessionIndex highest_session_;
    std::shared_ptr<SpamSlots> spam_slots_;
    std::shared_ptr<Participation> participation_;
    std::unique_ptr<RollingSessionWindow> rolling_session_window_;

    // This tracks only rolling session window failures.
    std::optional<SessionObtainingError> error_;

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

    /// Delay timer for establishing the rate limit.
    std::optional<clock::BasicWaitableTimer> rate_limit_timer_;

    std::shared_ptr<ThreadPool> int_pool_;
    std::shared_ptr<ThreadHandler> internal_context_;

    std::unique_ptr<RuntimeInfo> runtime_info_;

    /// Currently active batches of imports per candidate.
    std::unique_ptr<Batches> batches_;

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

    log::Logger log_ = log::createLogger("DisputeCoordinator", "dispute");
  };

}  // namespace kagome::dispute

#endif  // KAGOME_DISPUTE_DISPUTECOORDINATORIMPL
