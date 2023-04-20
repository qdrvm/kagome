/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_DISPUTE_DISPUTECOORDINATOR
#define KAGOME_DISPUTE_DISPUTECOORDINATOR

#include "dispute_coordinator/types.hpp"

namespace kagome::dispute {

  class DisputeCoordinator {
   public:
    virtual ~DisputeCoordinator() = default;

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
    virtual outcome::result<void> onImportStatements(
        /// The candidate receipt itself.
        CandidateReceipt candidate_receipt,

        /// The session the candidate appears in.
        SessionIndex session,

        /// Statements, with signatures checked, by validators participating in
        /// disputes.
        ///
        /// The validator index passed alongside each statement should
        /// correspond to the index of the validator in the set.
        std::vector<Indexed<SignedDisputeStatement>> statements,

        /// Inform the requester once we finished importing (if a sender was
        /// provided).
        ///
        /// This is:
        /// - we discarded the votes because
        ///   - they were ancient or otherwise invalid (result: `InvalidImport`)
        ///   - or we were not able to recover availability for an unknown
        ///   candidate (result: `InvalidImport`)
        ///   - or were known already (in that case the result will still be
        ///   `ValidImport`)
        /// - or we recorded them because (`ValidImport`)
        ///   - we cast our own vote already on that dispute
        ///   - or we have approval votes on that candidate
        ///   - or other explicit votes on that candidate already recorded
        ///   - or recovered availability for the candidate
        ///   - or the imported statements are backing/approval votes, which are
        ///   always accepted.
        std::optional<std::function<void(outcome::result<void>)>>
            pending_confirmation) = 0;
  };

}  // namespace kagome::dispute

#endif  // KAGOME_DISPUTE_DISPUTECOORDINATOR
