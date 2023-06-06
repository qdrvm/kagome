/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_DISPUTE_QUEUESIMPL
#define KAGOME_DISPUTE_QUEUESIMPL

#include "dispute_coordinator/participation/queues.hpp"

#include <map>

namespace kagome::blockchain {
  class BlockHeaderRepository;
}

namespace kagome::runtime {
  class ParachainHost;
}

namespace kagome::dispute {

  /// What can go wrong when queuing a request.
  enum class QueueError {
    /// Best effort queue was already full
    BestEffortFull,
    /// Priority queue was already full
    PriorityFull,
  };

  /// `Comparator` for ordering of disputes for candidates.
  ///
  /// This `comparator` makes it possible to order disputes based on age and to
  /// ensure some fairness between chains in case of equally old disputes.
  ///
  /// Objective ordering between nodes is important in case of lots disputes, so
  /// nodes will pull in the same direction and work on resolving the same
  /// disputes first. This ensures that we will conclude some disputes, even if
  /// there are lots of them. While any objective ordering would suffice for
  /// this goal, ordering by age ensures we are not only resolving disputes, but
  /// also resolve the oldest one first, which are also the most urgent and
  /// important ones to resolve.
  ///
  /// Note: That by `oldest` we mean oldest in terms of relay chain block
  /// number, for any block number that has not yet been finalized. If a block
  /// has been finalized already it should be treated as low priority when it
  /// comes to disputes, as even in the case of a negative outcome, we are
  /// already too late. The ordering mechanism here serves to prevent this from
  /// happening in the first place.
  struct CandidateComparator {
    /// Block number of the relay parent. It's wrapped in an `Option<>` because
    /// there are cases when it can't be obtained. For example when the node is
    /// lagging behind and new leaves are received with a slight delay.
    /// Candidates with unknown relay parent are treated with the lowest
    /// priority.
    ///
    /// The order enforced by `CandidateComparator` is important because we want
    /// to participate in the oldest disputes first.
    ///
    /// Note: In theory it would make more sense to use the `BlockNumber` of the
    /// including block, as inclusion time is the actual relevant event when it
    /// comes to ordering. The problem is, that a candidate can get included
    /// multiple times on forks, so the `BlockNumber` of the including block is
    /// not unique. We could theoretically work around that problem, by just
    /// using the lowest `BlockNumber` of all available including blocks - the
    /// problem is, that is not stable. If a new fork appears after the fact, we
    /// would start ordering the same candidate differently, which would result
    /// in the same candidate getting queued twice.
    std::optional<primitives::BlockNumber> relay_parent_block_number;

    /// By adding the `CandidateHash`, we can guarantee a unique ordering across
    /// candidates with the same relay parent block number. Candidates without
    /// `relay_parent_block_number` are ordered by the `candidate_hash` (and
    /// treated with the lowest priority, as already mentioned).
    CandidateHash candidate_hash;

    bool operator<(const CandidateComparator &other) const {
      if (relay_parent_block_number.value()
          == other.relay_parent_block_number.value()) {
        // if the relay parent is the same for both -> compare hashes
        return candidate_hash < other.candidate_hash;
      } else if (relay_parent_block_number.has_value()
                 xor other.relay_parent_block_number.has_value()) {
        // Candidates with known relay parents are always with priority
        return not relay_parent_block_number.has_value();
      } else {
        // Otherwise compare by number
        return relay_parent_block_number.value()
             < other.relay_parent_block_number.value();
      }
    }
  };

  /// Queues for dispute participation.
  /// In both queues we have a strict ordering of candidates and participation
  /// will happen in that order. Refer to `CandidateComparator` for details on
  /// the ordering.
  class QueuesImpl final : public Queues {
   public:
    static const size_t kPriorityQueueSize = 20'000;
    static const size_t kBestEffortQueueSize = 100;

    QueuesImpl(std::shared_ptr<blockchain::BlockHeaderRepository>
                   block_header_repository,
               std::shared_ptr<crypto::Hasher> hasher,
               std::shared_ptr<runtime::ParachainHost> api);

    outcome::result<void> queue(ParticipationPriority priority,
                                ParticipationRequest request) override;

    std::optional<ParticipationRequest> dequeue() override;

    outcome::result<void> prioritize_if_present(
        const CandidateReceipt &receipt) override;

   private:
    std::shared_ptr<blockchain::BlockHeaderRepository> block_header_repository_;
    std::shared_ptr<crypto::Hasher> hasher_;
    std::shared_ptr<runtime::ParachainHost> api_;

    /// Set of best effort participation requests.
    std::map<CandidateComparator, ParticipationRequest> best_effort_;

    /// Priority queue.
    std::map<CandidateComparator, ParticipationRequest> priority_;
  };

}  // namespace kagome::dispute

OUTCOME_HPP_DECLARE_ERROR(kagome::dispute, QueueError);

#endif
