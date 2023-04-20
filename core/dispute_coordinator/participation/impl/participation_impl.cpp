/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dispute_coordinator/participation/impl/participation_impl.hpp"

#include "dispute_coordinator/participation/queues.hpp"

namespace kagome::dispute {

  outcome::result<void> ParticipationImpl::queue_participation(
      ParticipationPriority priority, ParticipationRequest request) {
    // Participation already running - we can ignore that request:
    if (running_participations_.count(request.candidate_hash)) {
      return outcome::success();
    }

    // Available capacity - participate right away (if we already have a recent
    // block):
    if (recent_block_.has_value()) {
      if (running_participations_.size() < kMaxParallelParticipations) {
        return fork_participation(request, recent_block_->hash);
      }
    }

    // Out of capacity/no recent block yet - queue:
    return queue_->queue(priority, request);
  }

  outcome::result<void> ParticipationImpl::fork_participation(
      ParticipationRequest request, primitives::BlockHash recent_head) {
    if (running_participations_.emplace(request.candidate_hash).second) {

      // see: {polkadot}/node/core/dispute-coordinator/src/participation/mod.rs:255
      throw std::runtime_error("not implemented"); // FIXME

      // spawn
      // "participation-worker",
      // participate(self.worker_sender.clone(), sender, recent_head, req)
    }
    return outcome::success();
  }

}  // namespace kagome::dispute
