/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>
#include <vector>

namespace kagome::parachain {

  /// Tracks our impression of a single peer's view of the candidates a
  /// validator has seconded for a given relay-parent.
  ///
  /// It is expected to receive at most `VC_THRESHOLD` from us and be aware of
  /// at most `VC_THRESHOLD` via other means.
  struct VcPerPeerTracker {
    static constexpr size_t kTrackerThreshold = 2;

    VcPerPeerTracker(const log::Logger &logger) : logger_(logger) {
      BOOST_ASSERT(logger_);
      local_observed.reserve(kTrackerThreshold);
      remote_observed.reserve(kTrackerThreshold);
    }

    /// Note that the remote should now be aware that a validator has seconded a
    /// given candidate (by hash) based on a message that we have sent it from
    /// our local pool.
    void note_local(const network::CandidateHash &h) {
      if (!note_hash(local_observed, h)) {
        logger_->warn(
            "Statement distribution is erroneously attempting to distribute "
            "more than {} candidate(s) per validator index. Ignoring.",
            kTrackerThreshold);
      }
    }

    /// Note that the remote should now be aware that a validator has seconded a
    /// given candidate (by hash) based on a message that it has sent us.
    ///
    /// Returns `true` if the peer was allowed to send us such a message,
    /// `false` otherwise.
    bool note_remote(const network::CandidateHash &hash) {
      return note_hash(remote_observed, hash);
    }

    /// Returns `true` if the peer is allowed to send us such a message, `false`
    /// otherwise.
    bool is_wanted_candidate(const network::CandidateHash &hash) {
      return !contains(remote_observed, hash) && !is_full(remote_observed);
    }

   private:
    using CandidateHashPool = std::vector<network::CandidateHash>;

    bool contains(CandidateHashPool &pool, const network::CandidateHash &hash) {
      for (const auto &h : pool) {
        if (h == hash) {
          return true;
        }
      }
      return false;
    }

    bool is_full(CandidateHashPool &pool) {
      return pool.size() == pool.capacity();
    }

    bool note_hash(CandidateHashPool &pool,
                   const network::CandidateHash &hash) {
      std::unique_ptr<CandidateHashPool, void (*)(CandidateHashPool *)> _keeper(
          &pool, [](CandidateHashPool *pool) {
            BOOST_ASSERT(pool != nullptr);
            BOOST_ASSERT(pool->capacity() == kTrackerThreshold);
          });

      if (contains(pool, hash)) {
        return true;
      }

      if (!is_full(pool)) {
        pool.push_back(hash);
        return true;
      }
      return false;
    }

    CandidateHashPool local_observed;
    CandidateHashPool remote_observed;
    log::Logger logger_;
  };

}  // namespace kagome::parachain
