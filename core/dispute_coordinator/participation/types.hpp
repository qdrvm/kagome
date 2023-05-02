/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_DISPUTE_PARTICIPATION_TYPES_HPP
#define KAGOME_DISPUTE_PARTICIPATION_TYPES_HPP

#include "dispute_coordinator/types.hpp"

namespace kagome::dispute {

  /// A dispute participation request that can be queued.
  struct ParticipationRequest {
    CandidateHash candidate_hash;
    CandidateReceipt candidate_receipt;
    SessionIndex session;
  };

  /// Whether a `ParticipationRequest` should be put on best-effort or the
  /// priority queue.
  enum class ParticipationPriority : bool {
    BestEffort = false,
    Priority = true
  };

  /// Outcome of the validation process.
  enum ParticipationOutcome {
    /// Candidate was found to be valid.
    Valid,
    /// Candidate was found to be invalid.
    Invalid,
    /// Candidate was found to be unavailable.
    Unavailable,
    /// Something went wrong (bug), details can be found in the logs.
    Error,
  };

  /// Statement as result of the validation process.
  struct ParticipationStatement {
    /// Relevant session.
    SessionIndex session;
    /// The candidate the worker has been spawned for.
    CandidateHash candidate_hash;
    /// Used receipt.
    CandidateReceipt candidate_receipt;
    /// Actual result.
    ParticipationOutcome outcome;
  };

}  // namespace kagome::dispute

#endif  // KAGOME_DISPUTE_PARTICIPATION_TYPES_HPP
