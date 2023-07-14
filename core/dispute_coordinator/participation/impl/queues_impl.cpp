/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dispute_coordinator/participation/impl/queues_impl.hpp"

#include "blockchain/block_header_repository.hpp"
#include "blockchain/block_tree_error.hpp"
#include "runtime/runtime_api/parachain_host.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::dispute, QueueError, e) {
  using E = kagome::dispute::QueueError;
  switch (e) {
    case E::BestEffortFull:
      return "Request could not be queued, because best effort queue was "
             "already full.";
    case E::PriorityFull:
      return "Request could not be queued, because priority queue was already "
             "full.";
  }
  return "unknown error (dispute::QueueError)";
}

namespace kagome::dispute {

  QueuesImpl::QueuesImpl(std::shared_ptr<blockchain::BlockHeaderRepository>
                             block_header_repository,
                         std::shared_ptr<crypto::Hasher> hasher,
                         std::shared_ptr<runtime::ParachainHost> api)
      : block_header_repository_(std::move(block_header_repository)),
        hasher_(std::move(hasher)),
        api_(std::move(api)) {
    BOOST_ASSERT(block_header_repository_);
    BOOST_ASSERT(hasher_);
    BOOST_ASSERT(api_);
  }

  outcome::result<void> QueuesImpl::queue(ParticipationPriority priority,
                                          ParticipationRequest request) {
    const auto &candidate_hash = request.candidate_receipt.hash(*hasher_);
    auto block_number_res = block_header_repository_->getNumberByHash(
        request.candidate_receipt.descriptor.relay_parent);

    std::optional<primitives::BlockNumber> block_number_opt{};
    if (block_number_res.has_value()) {
      block_number_opt = block_number_res.value();
    } else if (block_number_res
               != outcome::failure(
                   blockchain::BlockTreeError::HEADER_NOT_FOUND)) {
      return block_number_res.as_failure();
    } else {
      // LOG-WARN: "Candidate's relay_parent could not be found via chain API -
      //            `CandidateComparator` with an empty relay parent block
      //            number will be provided!"
    }

    CandidateComparator comparator{
        .relay_parent_block_number = block_number_opt,
        .candidate_hash = candidate_hash};

    if (priority == ParticipationPriority::Priority) {
      if (priority_.size() >= kPriorityQueueSize) {
        return QueueError::PriorityFull;
      }
      // Remove any best effort entry:
      best_effort_.erase(comparator);

      priority_.emplace(comparator, request);
    } else {
      if (priority_.count(comparator)) {
        // The candidate is already in priority queue - don't add in in best
        // effort too.
        return outcome::success();
      }

      if (best_effort_.size() >= kBestEffortQueueSize) {
        return QueueError::BestEffortFull;
      }

      best_effort_.emplace(comparator, request);
    }

    return outcome::success();
  }

  std::optional<ParticipationRequest> QueuesImpl::dequeue() {
    if (not priority_.empty()) {
      if (auto priority_node =
              priority_.extract((std::next(priority_.rbegin()).base()))) {
        return std::move(priority_node.mapped());
      }
    }

    if (not best_effort_.empty()) {
      if (auto best_effort_node =
              best_effort_.extract((std::next(best_effort_.rbegin()).base()))) {
        return std::move(best_effort_node.mapped());
      }
    }

    return std::nullopt;
  }

  outcome::result<void> QueuesImpl::prioritize_if_present(
      const CandidateReceipt &receipt) {
    const auto &candidate_hash = receipt.hash(*hasher_);
    auto block_number_res = block_header_repository_->getNumberByHash(
        receipt.descriptor.relay_parent);

    std::optional<primitives::BlockNumber> block_number_opt{};
    if (block_number_res.has_value()) {
      block_number_opt = block_number_res.value();
    } else if (block_number_res
               != outcome::failure(
                   blockchain::BlockTreeError::HEADER_NOT_FOUND)) {
      return block_number_res.as_failure();
    } else {
      // LOG-WARN: "Candidate's relay_parent could not be found via chain API -
      //            `CandidateComparator` with an empty relay parent block
      //            number will be provided!"
    }

    CandidateComparator comparator{
        .relay_parent_block_number = block_number_opt,
        .candidate_hash = candidate_hash};

    if (priority_.size() >= kPriorityQueueSize) {
      return QueueError::PriorityFull;
    }

    if (auto it = best_effort_.find(comparator); it != best_effort_.end()) {
      priority_.emplace(comparator, it->second);
    }

    return outcome::success();
  }

}  // namespace kagome::dispute
