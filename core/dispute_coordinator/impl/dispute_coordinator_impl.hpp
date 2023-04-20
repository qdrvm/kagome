/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_DISPUTE_DISPUTECOORDINATORIMPL
#define KAGOME_DISPUTE_DISPUTECOORDINATORIMPL

#include "dispute_coordinator/dispute_coordinator.hpp"

#include "crypto/crypto_store/session_keys.hpp"
#include "crypto/sr25519_provider.hpp"
#include "dispute_coordinator/chain_scraper.hpp"
#include "dispute_coordinator/dispute_message.hpp"
#include "dispute_coordinator/impl/candidate_vote_state.hpp"
#include "dispute_coordinator/rolling_session_window.hpp"
#include "dispute_coordinator/spam_slots.hpp"
#include "dispute_coordinator/storage.hpp"
#include "dispute_coordinator/types.hpp"
#include "log/logger.hpp"
#include "parachain/types.hpp"

namespace kagome::dispute {
  class ChainScraper;
  class LocalKeystore;
  class Participation;
}  // namespace kagome::dispute

namespace kagome::dispute {

  class DisputeCoordinatorImpl final : public DisputeCoordinator {
   public:
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

    std::shared_ptr<clock::SystemClock> clock_;
    std::shared_ptr<LocalKeystore> keystore_;
    std::shared_ptr<crypto::SessionKeys> session_keys_;
    std::shared_ptr<Storage> storage_;
    std::shared_ptr<crypto::Sr25519Provider> sr25519_crypto_provider_;

    std::shared_ptr<ChainScraper> scraper_;
    std::shared_ptr<SpamSlots> spam_slots_;
    std::shared_ptr<Participation> participation_;

    std::shared_ptr<RollingSessionWindow> rolling_session_window_;

    log::Logger log_ = log::createLogger("DisputeCoordinator", "dispute");
  };

}  // namespace kagome::dispute

#endif  // KAGOME_DISPUTE_DISPUTECOORDINATORIMPL
