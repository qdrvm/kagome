/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "dispute_coordinator/participation/queues.hpp"

#include <map>

#include "dispute_coordinator/participation/types.hpp"
#include "outcome/outcome.hpp"

namespace kagome::dispute {
  /// Queues for dispute participation.
  /// In both queues we have a strict ordering of candidates and participation
  /// will happen in that order.
  /// Refer to `CandidateComparator` for details on the ordering.
  class Queues {
   public:
    virtual ~Queues() = default;

    /// Will put message in queue, either priority or best effort depending on
    /// priority.
    ///
    /// If the message was already previously present on best effort, it will be
    /// moved to priority if it is considered priority now.
    ///
    /// Returns error in case a queue was found full already.
    virtual outcome::result<void> queue(ParticipationPriority priority,
                                        ParticipationRequest request) = 0;

    /// Get the next best request for dispute participation if any.
    /// First the priority queue is considered and then the best effort one.
    virtual std::optional<ParticipationRequest> dequeue() = 0;

    /// Reprioritizes any participation requests pertaining to the passed
    /// candidates from best effort to priority.
    virtual outcome::result<void> prioritize_if_present(
        const CandidateReceipt &receipt) = 0;
  };

}  // namespace kagome::dispute
