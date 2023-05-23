/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_DISPUTE_DISPUTECOORDINATORIMPL
#define KAGOME_DISPUTE_DISPUTECOORDINATORIMPL

#include "dispute_coordinator/dispute_coordinator.hpp"
#include "network/dispute_request_observer.hpp"

#include "crypto/crypto_store/session_keys.hpp"
#include "crypto/sr25519_provider.hpp"
#include "dispute_coordinator/chain_scraper.hpp"
#include "dispute_coordinator/impl/candidate_vote_state.hpp"
#include "dispute_coordinator/impl/errors.hpp"
#include "dispute_coordinator/participation/types.hpp"
#include "dispute_coordinator/rolling_session_window.hpp"
#include "dispute_coordinator/spam_slots.hpp"
#include "dispute_coordinator/storage.hpp"
#include "dispute_coordinator/types.hpp"
#include "log/logger.hpp"
#include "network/peer_view.hpp"
#include "parachain/types.hpp"

namespace kagome {
  class ThreadPool;
  class ThreadHandler;
}  // namespace kagome

namespace kagome::application {
  class AppStateManager;
}

namespace kagome::dispute {
  class ChainScraper;
  class Participation;
}  // namespace kagome::dispute

namespace kagome::network {
  struct DisputeMessage;
}

namespace kagome::parachain {
  struct ApprovalDistribution;
}

namespace kagome::runtime {
  class ParachainHost;
}

namespace kagome::dispute {

  class DisputeCoordinatorImpl final
      : public DisputeCoordinator,
        public network::DisputeRequestObserver,
        public std::enable_shared_from_this<DisputeCoordinatorImpl> {
    static constexpr Timestamp kActiveDurationSecs = 180;

   public:
    DisputeCoordinatorImpl(
        std::shared_ptr<application::AppStateManager> app_state_manager,
        std::shared_ptr<libp2p::basic::Scheduler> scheduler,
        std::shared_ptr<clock::SystemClock> clock,
        std::shared_ptr<crypto::SessionKeys> session_keys,
        std::shared_ptr<Storage> storage,
        std::shared_ptr<crypto::Sr25519Provider> sr25519_crypto_provider,
        std::shared_ptr<crypto::Hasher> hasher,
        std::shared_ptr<blockchain::BlockTree> block_tree,
        std::shared_ptr<runtime::ParachainHost> api,
        std::shared_ptr<parachain::ApprovalDistribution> approval_distribution);

    bool prepare();
    bool start();
    void stop();

    void onDisputeRequest(const network::DisputeMessage &message) override;

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
    outcome::result<DisputeMessage> make_dispute_message(
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

    std::shared_ptr<application::AppStateManager> app_state_manager_;
    std::shared_ptr<libp2p::basic::Scheduler> scheduler_;
    std::shared_ptr<clock::SystemClock> clock_;
    std::shared_ptr<crypto::SessionKeys> session_keys_;
    std::shared_ptr<Storage> storage_;
    std::shared_ptr<crypto::Sr25519Provider> sr25519_crypto_provider_;
    std::shared_ptr<crypto::Hasher> hasher_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<runtime::ParachainHost> api_;
    std::shared_ptr<parachain::ApprovalDistribution> approval_distribution_;

    std::shared_ptr<network::PeerView> peer_view_;
    std::shared_ptr<network::PeerView::MyViewSubscriber> my_view_sub_;
    std::shared_ptr<primitives::events::ChainEventSubscriber> chain_sub_;

    std::atomic_bool initialized_ = false;

    libp2p::basic::Scheduler::Handle processing_loop_handle_;

    std::shared_ptr<ChainScraper> scraper_;
    SessionIndex highest_session_;
    std::shared_ptr<SpamSlots> spam_slots_;
    std::shared_ptr<Participation> participation_;
    std::unique_ptr<RollingSessionWindow> rolling_session_window_;

    // This tracks only rolling session window failures.
    std::optional<RollingSessionWindowError> error_;

    //
    std::shared_ptr<ThreadPool> int_pool_;
    std::shared_ptr<ThreadHandler> internal_context_;

    log::Logger log_ = log::createLogger("DisputeCoordinator", "dispute");
  };

}  // namespace kagome::dispute

#endif  // KAGOME_DISPUTE_DISPUTECOORDINATORIMPL
