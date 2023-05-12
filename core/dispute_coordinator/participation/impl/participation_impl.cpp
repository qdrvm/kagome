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
      // see:
      // {polkadot}/node/core/dispute-coordinator/src/participation/mod.rs:255
      throw std::runtime_error("not implemented");  // FIXME

      // spawn
      // "participation-worker",
      // participate(self.worker_sender.clone(), sender, recent_head, req)
    }
    return outcome::success();
  }

  /// Dequeue until `MAX_PARALLEL_PARTICIPATIONS` is reached.
  outcome::result<void> ParticipationImpl::dequeue_until_capacity(
      const primitives::BlockHash &recent_head) {
    while (running_participations_.size() < kMaxParallelParticipations) {
      if (let Some(req) = self.queue.dequeue()) {
        fork_participation(req, recent_head);
      } else {
        break;
      }
    }
    return outcome::success();
  }

  outcome::result<void> ParticipationImpl::process_active_leaves_update(
      const ActiveLeavesUpdate &update) {
    // https://github.com/paritytech/polkadot/blob/40974fb99c86f5c341105b7db53c7aa0df707d66/node/core/dispute-coordinator/src/participation/mod.rs#L197

    if (update.activated.has_value()) {
      const auto &activated = update.activated.value();

      if (not recent_block_.has_value()) {
        recent_block_ = std::make_optional<primitives::BlockInfo>(
            activated.number, activated.hash);
        // Work got potentially unblocked:
        dequeue_until_capacity(activated.hash);

      } else if (activated.number > recent_block_->number) {
        recent_block_ = std::make_optional<primitives::BlockInfo>(
            activated.number, activated.hash);
      }
    }

    return outcome::success();
  }

  outcome::result<void> ParticipationImpl::get_participation_result(
      const ParticipationStatement &msg) {
    // https://github.com/paritytech/polkadot/blob/40974fb99c86f5c341105b7db53c7aa0df707d66/node/core/dispute-coordinator/src/participation/mod.rs#L181
    running_participations_.erase(msg.candidate_hash);

    BOOST_ASSERT_MSG(
        recent_block_.has_value(),
        "We never ever reset recent_block to `None` and we already "
        "received a result, so it must have been set before. qed.");
    auto recent_block = recent_block_.value();

    return dequeue_until_capacity(recent_block.hash);
  }

  outcome::result<void> ParticipationImpl::bump_to_priority_for_candidates(
      std::vector<CandidateReceipt> &included_receipts) {
    for (auto &receipt : included_receipts) {
      OUTCOME_TRY(queue_->prioritize_if_present(receipt));
    }
    return outcome::success();
  }

}  // namespace kagome::dispute
