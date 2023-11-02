/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/production_consensus.hpp"

namespace kagome::consensus {
  /**
   * Slow down block production proportionally to finality lag.
   * https://github.com/paritytech/substrate/blob/50de15d8740a129db9c18a6698fbd183b00326a2/client/consensus/slots/src/lib.rs#L772-L806
   */
  inline bool backoff(const ProductionConsensus &consensus,
                      const primitives::BlockHeader &best,
                      primitives::BlockNumber finalized,
                      SlotNumber slot) {
    constexpr int64_t kMaxInterval = 100;
    constexpr auto kUnfinalizedSlack = 50;
    constexpr auto kAuthoringBias = 2;

    auto slot_res = consensus.getSlot(best);
    if (not slot_res) {
      return false;
    }
    auto best_slot = slot_res.value();
    if (slot <= best_slot) {
      return false;
    }
    auto interval = (static_cast<int64_t>(best.number)
                     - static_cast<int64_t>(finalized) - kUnfinalizedSlack)
                  / kAuthoringBias;
    return static_cast<int64_t>(slot - best_slot)
        <= std::min(interval, kMaxInterval);
  }
}  // namespace kagome::consensus
