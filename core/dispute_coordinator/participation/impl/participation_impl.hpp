/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_DISPUTE_PARTICIPATIONIMPL
#define KAGOME_DISPUTE_PARTICIPATIONIMPL

#include "dispute_coordinator/participation/impl/queues_impl.hpp"
#include "dispute_coordinator/participation/participation.hpp"
#include "utils/thread_pool.hpp"

#include <unordered_set>

namespace kagome {
  class ThreadHandler;
}

namespace kagome::dispute {

  class ParticipationImpl final : Participation {
   public:
    static const size_t kMaxParallelParticipations = 3;

    ParticipationImpl(std::shared_ptr<ThreadHandler> internal_context)
        : internal_context_(std::move(internal_context)) {
      BOOST_ASSERT(internal_context != nullptr);
    }

    outcome::result<void> queue_participation(
        ParticipationPriority priority, ParticipationRequest request) override;

    outcome::result<void> fork_participation(
        ParticipationRequest request,
        primitives::BlockHash recent_head) override;

    outcome::result<void> process_active_leaves_update(
        const ActiveLeavesUpdate &update) override;

    outcome::result<void> get_participation_result(
        const ParticipationStatement &msg) override;

    outcome::result<void> bump_to_priority_for_candidates(
        std::vector<CandidateReceipt> &included_receipts) override;

   private:
    /// Dequeue until `MAX_PARALLEL_PARTICIPATIONS` is reached.
    outcome::result<void> dequeue_until_capacity(
        const primitives::BlockHash &recent_head);

    std::shared_ptr<ThreadHandler> internal_context_;

    /// Participations currently being processed.
    std::unordered_set<CandidateHash> running_participations_;
    /// Priority and best effort queues.
    std::unique_ptr<Queues> queue_;
    /// Sender to be passed to worker tasks.
    // std::unique_ptr<WorkerMessageSender> worker_sender_;
    /// Some recent block for retrieving validation code from chain.
    std::optional<primitives::BlockInfo> recent_block_;
  };

}  // namespace kagome::dispute

#endif  // KAGOME_DISPUTE_PARTICIPATIONIMPL
