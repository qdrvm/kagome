/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "dispute_coordinator/participation/impl/queues_impl.hpp"
#include "dispute_coordinator/participation/participation.hpp"
#include "utils/thread_pool.hpp"

#include <unordered_set>

namespace kagome {
  class ThreadHandler;
}

namespace kagome::parachain {
  class Recovery;
  class Pvf;
}  // namespace kagome::parachain

namespace kagome::dispute {
  class DisputeCoordinator;
}

namespace kagome::dispute {

  class ParticipationImpl final
      : public Participation,
        public std::enable_shared_from_this<ParticipationImpl> {
   public:
    static const size_t kMaxParallelParticipations = 3;

    ParticipationImpl(std::shared_ptr<blockchain::BlockHeaderRepository>
                          block_header_repository,
                      std::shared_ptr<crypto::Hasher> hasher,
                      std::shared_ptr<runtime::ParachainHost> api,
                      std::shared_ptr<parachain::Recovery> recovery,
                      std::shared_ptr<parachain::Pvf> pvf,
                      std::shared_ptr<ThreadHandler> internal_context,
                      std::weak_ptr<DisputeCoordinator> dispute_coordinator);

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
    void participate(ParticipationRequest request,
                     primitives::BlockHash recent_head);

    struct ParticipationContext {
      ParticipationRequest request;
      primitives::BlockHash block_hash;
      std::optional<runtime::AvailableData> available_data{};
      std::optional<runtime::ValidationCode> validation_code{};
    };
    using ParticipationContextPtr = std::shared_ptr<ParticipationContext>;
    using ParticipationCallback = std::function<void(ParticipationOutcome)>;

    void participate_stage1(ParticipationContextPtr ctx,
                            ParticipationCallback &&cb);
    void participate_stage2(ParticipationContextPtr ctx,
                            ParticipationCallback &&cb);
    void participate_stage3(ParticipationContextPtr ctx,
                            ParticipationCallback &&cb);

    /// Dequeue until `MAX_PARALLEL_PARTICIPATIONS` is reached.
    outcome::result<void> dequeue_until_capacity(
        const primitives::BlockHash &recent_head);

    std::shared_ptr<blockchain::BlockHeaderRepository> block_header_repository_;
    std::shared_ptr<runtime::ParachainHost> api_;
    std::shared_ptr<parachain::Recovery> recovery_;
    std::shared_ptr<parachain::Pvf> pvf_;
    std::shared_ptr<ThreadHandler> internal_context_;
    std::weak_ptr<DisputeCoordinator> dispute_coordinator_;

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
