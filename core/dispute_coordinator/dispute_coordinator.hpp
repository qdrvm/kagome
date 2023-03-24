/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_DISPUTE_DISPUTECOORDINATOR
#define KAGOME_DISPUTE_DISPUTECOORDINATOR

#include "network/types/collator_messages.hpp"
#include "parachain/types.hpp"

namespace kagome::dispute {

  using network::CandidateReceipt;
  using network::DisputeStatement;
  using network::SessionIndex;
  using network::ValidatorIndex;
  using network::Vote;
  using parachain::CandidateHash;
  using parachain::ValidatorSignature;

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
        ) = 0;
  };

}  // namespace kagome::dispute

#endif  // KAGOME_DISPUTE_DISPUTECOORDINATOR
