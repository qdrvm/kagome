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
    outcome::result<void> queue_participation(ParticipationPriority priority,
                                              ParticipationRequest request) = 0;
  };

}  // namespace kagome::dispute

#endif  // KAGOME_DISPUTE_PARTICIPATION
