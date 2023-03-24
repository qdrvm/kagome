/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_DISPUTE_DISPUTECOORDINATORIMPL
#define KAGOME_DISPUTE_DISPUTECOORDINATORIMPL

#include "dispute_coordinator/dispute_coordinator.hpp"

#include "dispute_coordinator/rolling_session_window.hpp"
#include "dispute_coordinator/storage.hpp"
#include "parachain/types.hpp"
#include "log/logger.hpp"

namespace kagome::dispute {

  class DisputeCoordinatorImpl final : public DisputeCoordinator {
    using ValidatorId = network::ValidatorId;

    template <typename T>
    using Indexed = parachain::Indexed<T>;

    template <typename T>
    using IndexedIndexedAndSigned = parachain::IndexedAndSigned<T>;

   public:
    /// Import a statement by a validator about a candidate.
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
    outcome::result<void> onImportStatements(
        /// The hash of the candidate.
        CandidateHash candidate_hash,
        /// The candidate receipt itself.
        CandidateReceipt candidate_receipt,
        /// The session the candidate appears in.
        SessionIndex session,
        /// Triples containing the following:
        /// - A statement, either indicating validity or invalidity of the
        /// candidate.
        /// - The validator index (within the session of the candidate) of the
        /// validator casting the vote.
        /// - The signature of the validator casting the vote.
        std::vector<Vote  // std::tuple<DisputeStatement, ValidatorIndex,
                          // ValidatorSignature>
                    > statements

        /// Inform the requester once we finished importing.
        ///
        /// This is, we either discarded the votes, just record them because we
        /// casted our vote already or recovered availability for the candidate
        /// successfully.
        //, oneshot::Sender<ImportStatementsResult> pending_confirmation
        ) override;

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
    outcome::result<bool> handle_statements(
        CandidateHash candidate_hash,
        MaybeCandidateReceipt candidate_receipt,
        SessionIndex session,
        std::vector<Indexed<SignedDisputeStatement>> statements);

    void find_controlled_validator_indices(
        Indexed<std::vector<ValidatorId>> validators);

    std::shared_ptr<clock::SystemClock> clock_;
    std::shared_ptr<LocalKeystore> keystore_;
    std::shared_ptr<Storage> storage_;

    ChainScraper scraper_;

    std::shared_ptr<RollingSessionWindow> rolling_session_window_;

    log::Logger log_ = log::createLogger("DisputeCoordinator", "dispute");
  };

}  // namespace kagome::dispute

#endif  // KAGOME_DISPUTE_DISPUTECOORDINATORIMPL
