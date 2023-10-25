/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

namespace kagome::consensus::sassafras {

  enum class Phase {
    /// Validators generate and submit their candidate tickets along with
    /// validity proofs to the blockchain. The validity proof ensures that the
    /// tickets are legitimate while maintaining the anonymity of the
    /// authorship.
    SubmissionOfCandidateTickets,

    /// Submitted candidate tickets undergo a validation process to verify their
    /// authenticity, integrity and compliance with other protocol-specific
    /// rules.
    ValidationOfCandidateTickets,

    /// After collecting all candidate tickets, a deterministic method is
    /// employed to associate some of these tickets with specific slots.
    TicketAndSlotBinding,

    /// Validators assert ownership of tickets during the block production
    /// phase. This step establishes a secure binding between validators and
    /// their respective slots.
    ClaimOfTicketOwnership,

    /// During block verification, the claims of ticket ownership are rigorously
    /// validated to uphold the protocol's integrity.
    ValidationOfTicketOwnership,
  };

}
