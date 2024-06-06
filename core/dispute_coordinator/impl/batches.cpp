/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dispute_coordinator/impl/batches.hpp"

#include "dispute_coordinator/impl/errors.hpp"

namespace kagome::dispute {

  Batches::Batches(log::Logger logger,
                   clock::SteadyClock &clock,
                   std::shared_ptr<crypto::Hasher> hasher)
      : logger_(std::move(logger)), clock_(clock), hasher_(std::move(hasher)) {}

  outcome::result<std::tuple<std::shared_ptr<Batch>, bool>> Batches::find_batch(
      const CandidateHash &candidate_hash,
      const CandidateReceipt &candidate_receipt) {
    BOOST_ASSERT(candidate_hash == candidate_receipt.hash(*hasher_));

    auto it = batches_.find(candidate_hash);
    if (it != batches_.end()) {
      return {it->second, false};
    }

    if (batches_.size() >= 1000 /* kMaxBatchSize */) {  // FIXME
      return BatchError::MaxBatchLimitReached;
    }

    auto batch =
        batches_
            .emplace(candidate_hash,
                     std::make_shared<Batch>(
                         candidate_hash, candidate_receipt, clock_.now()))
            .first->second;
    return {batch, true};
  }

  std::vector<PreparedImport> Batches::check_batches() {
    std::vector<PreparedImport> imports;

    // Can't try to check if nothing waiting
    if (waiting_queue_.empty()) {
      return imports;
    }

    auto now = clock_.now();
    for (;;) {
      auto candidate_hash = waiting_queue_.front();

      auto it = batches_.find(candidate_hash);
      BOOST_ASSERT_MSG(
          it != batches_.end(),
          "Entries referenced in `waiting_queue` are supposed to exist!");

      auto &batch = it->second;
      if (batch->next_tick_time() > now) {
        break;
      }
      waiting_queue_.pop();

      auto prepared_import_opt = batch->tick(now);

      // Batch done
      if (prepared_import_opt.has_value()) {
        auto &prepared_import = prepared_import_opt.value();
        SL_TRACE(logger_, "Batch (candidate={}) became ready", candidate_hash);
        imports.push_back(std::move(prepared_import));
        batches_.erase(it);
        continue;
      }

      // Batch alive
      SL_TRACE(logger_,
               "Batch (candidate={}) found to be still alive on check",
               candidate_hash);
      waiting_queue_.emplace(candidate_hash);
    }

    return imports;
  }

}  // namespace kagome::dispute
