/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_DISPUTE_BATCHES
#define KAGOME_DISPUTE_BATCHES

#include "clock/clock.hpp"
#include "dispute_coordinator/impl/batch.hpp"
#include "dispute_coordinator/types.hpp"

#include <queue>

namespace kagome::dispute {

  /// Manage batches.
  ///
  /// - Batches can be found via `find_batch()` in order to add votes to
  /// them/check they exist.
  /// - Batches can be checked for being ready for flushing in order to import
  /// contained votes.
  class Batches final {
   public:
    Batches(clock::SteadyClock &clock, std::shared_ptr<crypto::Hasher> hasher);

    /// Find a particular batch.
    ///
    /// That is either find it, or we create it as reflected by the result
    /// `FoundBatch`.find_batch(
    outcome::result<std::tuple<std::shared_ptr<Batch>, bool>> find_batch(
        const CandidateHash &candidate_hash,
        const CandidateReceipt &candidate_receipt);

    /// Wait for the next `tick` to check for ready batches.
    ///
    /// This function blocks (returns `Poll::Pending`) until at least one batch
    /// can be checked for readiness meaning that `BATCH_COLLECTING_INTERVAL`
    /// has passed since the last check for that batch or it reached end of
    /// life.
    ///
    /// If this `Batches` instance is empty (does not actually contain any
    /// batches), then this function will always return `Poll::Pending`.
    ///
    /// Returns: A `Vec` of all `PreparedImport`s from batches that became
    /// ready.
    std::vector<PreparedImport> check_batches();

   private:
    clock::SteadyClock &clock_;
    std::shared_ptr<crypto::Hasher> hasher_;

    /// The batches we manage.
    ///
    /// Kept invariants:
    /// For each entry in `batches`, there exists an entry in `waiting_queue` as
    /// well - we wait on all batches!
    std::unordered_map<CandidateHash, std::shared_ptr<Batch>> batches_;

    /// Waiting queue for waiting for batches to become ready for `tick`.
    ///
    /// Kept invariants by `Batches`:
    /// For each entry in the `waiting_queue` there exists a corresponding entry
    /// in `batches`.
    std::queue<CandidateHash> waiting_queue_;
  };

}  // namespace kagome::dispute

#endif  // KAGOME_DISPUTE_BATCHES
