/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "participation_impl.hpp"

namespace kagome::dispute {

    outcome::result<void> ParticipationImpl::queue_participation(
        ParticipationPriority priority, ParticipationRequest request) {
      // Participation already running - we can ignore that request:
      if (running_participations_.count(request.candidate_hash)) {
        return outcome::success();
      }

      // Available capacity - participate right away (if we already have a recent block):
      if (recent_block_.has_value()) {
        if (running_participations_.size() < kMaxParallelParticipations) {
           return fork_participation(request, recent_block_->hash);
        }
      }

      // Out of capacity/no recent block yet - queue:
      queue_->queue(ctx.sender(), priority, request).await
    }

}  // namespace kagome::dispute
