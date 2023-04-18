/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_DISPUTE_QUEUESIMPL
#define KAGOME_DISPUTE_QUEUESIMPL

#include "dispute_coordinator/participation/queues.hpp"

#include <map>

namespace kagome::dispute {

  enum class QueueError {
    BestEffortFull,
    PriorityFull,
  };

  /// Queues for dispute participation.
  /// In both queues we have a strict ordering of candidates and participation
  /// will happen in that order. Refer to `CandidateComparator` for details on
  /// the ordering.
  class QueuesImpl final : public Queues {
   public:
    QueuesImpl() = default;

    outcome::result<void> queue(ParticipationPriority priority,
                                ParticipationRequest request) override;

   private:
    /// Set of best effort participation requests.
    std::map<CandidateComparator, ParticipationRequest> best_effort_;

    /// Priority queue.
    std::map<CandidateComparator, ParticipationRequest> priority_;
  };

}  // namespace kagome::dispute

OUTCOME_HPP_DECLARE_ERROR(kagome::dispute, QueueError);

#endif
