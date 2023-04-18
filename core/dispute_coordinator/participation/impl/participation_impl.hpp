/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_DISPUTE_PARTICIPATIONIMPL
#define KAGOME_DISPUTE_PARTICIPATIONIMPL

#include "dispute_coordinator/participation/participation.hpp"

#include <unordered_set>

namespace kagome::dispute {

  class ParticipationImpl final : Participation {
    outcome::result<void> queue_participation(
        ParticipationPriority priority, ParticipationRequest request) override;
    static const size_t kMaxParallelParticipations = 3;

   public:
    /// Participations currently being processed.
    std::unordered_set<CandidateHash> running_participations_;
    /// Priority and best effort queues.
    Queues queue_;
    /// Sender to be passed to worker tasks.
    WorkerMessageSender worker_sender_;
    /// Some recent block for retrieving validation code from chain.
    std::optional<primitives::BlockInfo> recent_block_;
  };

}  // namespace kagome::dispute

#endif  // KAGOME_DISPUTE_PARTICIPATIONIMPL
