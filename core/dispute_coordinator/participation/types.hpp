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

}  // namespace kagome::dispute

#endif  // KAGOME_DISPUTE_PARTICIPATION_TYPES_HPP
