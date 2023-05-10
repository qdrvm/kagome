/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_DISPUTE_DISPUTECOORDINATORIMPL
#define KAGOME_DISPUTE_DISPUTECOORDINATORIMPL

#include <libp2p/basic/scheduler.hpp>
#include "dispute_coordinator/dispute_coordinator.hpp"

#include "crypto/crypto_store/session_keys.hpp"
#include "crypto/sr25519_provider.hpp"
#include "dispute_coordinator/chain_scraper.hpp"
#include "dispute_coordinator/dispute_message.hpp"
#include "dispute_coordinator/impl/candidate_vote_state.hpp"
#include "dispute_coordinator/impl/errors.hpp"
#include "dispute_coordinator/rolling_session_window.hpp"
#include "dispute_coordinator/spam_slots.hpp"
#include "dispute_coordinator/storage.hpp"
#include "dispute_coordinator/types.hpp"
#include "log/logger.hpp"
#include "parachain/types.hpp"

namespace kagome::application {
  class AppStateManager;
}

namespace kagome::dispute {
  class ChainScraper;
  class LocalKeystore;
  class Participation;
}  // namespace kagome::dispute

namespace kagome::runtime {
  class ParachainHost;
}

namespace kagome::dispute {

  class DisputeCoordinatorImpl final : public DisputeCoordinator {
    static constexpr Timestamp kActiveDurationSecs = 180;

   public:
    DisputeCoordinatorImpl(
        std::shared_ptr<application::AppStateManager> app_state_manager,
        std::shared_ptr<libp2p::basic::Scheduler> scheduler,
        std::shared_ptr<clock::SystemClock> clock,
        std::shared_ptr<LocalKeystore> keystore,
        std::shared_ptr<crypto::SessionKeys> session_keys,
        std::shared_ptr<Storage> storage,
        std::shared_ptr<crypto::Sr25519Provider> sr25519_crypto_provider);

    bool start();
    void stop();

    outcome::result<void> onImportStatements(
        CandidateReceipt candidate_receipt,
        SessionIndex session,
        std::vector<Indexed<SignedDisputeStatement>> statements,
        std::optional<std::function<void(outcome::result<void>)>>
            pending_confirmation) override;

    /* clang-format off

    /// Fetch a list of all recent disputes that the coordinator is aware of.
    /// These are disputes which have occurred any time in recent sessions,
    /// which may have already concluded.
    void RecentDisputes(ResponseChannel<Vec<(SessionIndex, CandidateHash)>>);

    /// Fetch a list of all active disputes that the coordinator is aware of.
    /// These disputes are either unconcluded or recently concluded.
    void ActiveDisputes(ResponseChannel<Vec<(SessionIndex, CandidateHash)>>);

    /// Get candidate votes for a candidate.
    void QueryCandidateVotes(SessionIndex,
                             CandidateHash,
                             ResponseChannel<Option<CandidateVotes>>);

    /// Sign and issue local dispute votes. A value of `true` indicates
    /// validity, and `false` invalidity.
    void IssueLocalStatement(SessionIndex,
                             CandidateHash,
                             CandidateReceipt,
                             bool);

    /// Determine the highest undisputed block within the given chain, based on
    /// where candidates were included. If even the base block should not be
    /// finalized due to a dispute, then `None` should be returned on the
    /// channel.
    ///
    /// The block descriptions begin counting upwards from the block after the
    /// given `base_number`. The `base_number` is typically the number of the
    /// last finalized block but may be slightly higher. This block is
    /// inevitably going to be finalized so it is not accounted for by this
    /// function.
    void DetermineUndisputedChain{
      base_number : BlockNumber,
      block_descriptions : Vec<(BlockHash, SessionIndex, Vec<CandidateHash>)>,
      rx : ResponseSender<Option<(BlockNumber, BlockHash)>>,
    };

    clang-format on */

   private:
    void run();

    // Run the subsystem until an error is encountered or a `conclude` signal is
    // received. Most errors are non-fatal and should lead to another call to
    // this function.
    //
    // A return success indicates that an exit should be made, while non-fatal
    // errors lead to another call to this function.
    outcome::result<void> run_until_error();

    outcome::result<void> process_on_chain_votes(ScrapedOnChainVotes votes);

    outcome::result<void> process_active_leaves_update(
        const ActiveLeavesUpdate &update);

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

    void find_controlled_validator_indices(
        Indexed<std::vector<ValidatorId>> validators);

    bool is_potential_spam(const CandidateVoteState &vote_state,
                           const CandidateHash &candidate_hash);

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

    outcome::result<void> handle_incoming(
        const DisputeCoordinatorMessage &message);

    outcome::result<void> handle_incoming_ImportStatements(
        const ImportStatements &msg);
    outcome::result<void> handle_incoming_RecentDisputes(
        const RecentDisputesRequest_ &p);
    outcome::result<void> handle_incoming_ActiveDisputes(
        const ActiveDisputes &msg);
    outcome::result<void> handle_incoming_QueryCandidateVotes(
        const QueryCandidateVotes &msg);
    outcome::result<void> handle_incoming_IssueLocalStatement(
        const IssueLocalStatement &msg);
    outcome::result<void> handle_incoming_DetermineUndisputedChain(
        const DetermineUndisputedChain &msg);

    std::shared_ptr<application::AppStateManager> app_state_manager_;
    std::shared_ptr<libp2p::basic::Scheduler> scheduler_;
    std::shared_ptr<clock::SystemClock> clock_;
    std::shared_ptr<LocalKeystore> keystore_;
    std::shared_ptr<crypto::SessionKeys> session_keys_;
    std::shared_ptr<Storage> storage_;
    std::shared_ptr<crypto::Sr25519Provider> sr25519_crypto_provider_;

    libp2p::basic::Scheduler::Handle processing_loop_handle_;

    std::shared_ptr<ChainScraper> scraper_;
    SessionIndex highest_session_;
    std::shared_ptr<SpamSlots> spam_slots_;
    std::shared_ptr<Participation> participation_;
    std::shared_ptr<RollingSessionWindow> rolling_session_window_;

    // This tracks only rolling session window failures.
    std::optional<RollingSessionWindowError> error_;

    log::Logger log_ = log::createLogger("DisputeCoordinator", "dispute");
  };

}  // namespace kagome::dispute

#endif  // KAGOME_DISPUTE_DISPUTECOORDINATORIMPL
