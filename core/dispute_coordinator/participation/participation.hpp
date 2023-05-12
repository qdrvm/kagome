/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_DISPUTE_PARTICIPATION
#define KAGOME_DISPUTE_PARTICIPATION

#include "dispute_coordinator/participation/types.hpp"
#include "network/types/collator_messages.hpp"
#include "parachain/types.hpp"

namespace kagome::dispute {

  /// Keep track of disputes we need to participate in.
  ///
  /// - Prioritize and queue participations
  /// - Dequeue participation requests in order and launch participation worker.
  class Participation {
   public:
    virtual ~Participation() = default;

    /// Queue a dispute for the node to participate in.
    ///
    /// If capacity is available right now and we already got some relay chain
    /// head via `on_active_leaves_update`, the participation will be launched
    /// right away.
    ///
    /// Returns: false, if queues are already full.
    virtual outcome::result<void> queue_participation(
        ParticipationPriority priority, ParticipationRequest request) = 0;

    /// Fork a participation task in the background.
    virtual outcome::result<void> fork_participation(
        ParticipationRequest request, primitives::BlockHash recent_head) = 0;

    /// Process active leaves update.
    ///
    /// Make sure we to dequeue participations if that became possible and
    /// update most recent block.
    virtual outcome::result<void> process_active_leaves_update(
        const ActiveLeavesUpdate &update) = 0;

    /// Message from a worker task was received - get the outcome.
    ///
    /// Call this function to keep participations going and to receive
    /// `ParticipationStatement`s.
    ///
    /// This message has to be called for each received worker message, in order
    /// to make sure enough participation processes are running at any given
    /// time.
    ///
    /// Returns: The received `ParticipationStatement` or a fatal error, in case
    /// something went wrong when dequeuing more requests (tasks could not be
    /// spawned).
    virtual outcome::result<void> get_participation_result(
        const ParticipationStatement &msg) = 0;

    /// Moving any request concerning the given candidates from best-effort to
    /// priority, ignoring any candidates that don't have any queued
    /// participation requests.
    virtual outcome::result<void> bump_to_priority_for_candidates(
        std::vector<CandidateReceipt> &included_receipts) = 0;
  };

}  // namespace kagome::dispute

#endif  // KAGOME_DISPUTE_PARTICIPATION
