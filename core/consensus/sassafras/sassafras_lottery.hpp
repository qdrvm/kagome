/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <optional>

#include "consensus/sassafras/types/randomness.hpp"
#include "consensus/sassafras/types/ticket.hpp"
#include "consensus/timeline/types.hpp"
#include "crypto/bandersnatch_types.hpp"

namespace kagome::consensus::sassafras {

  class SassafrasLottery {
   public:
    virtual ~SassafrasLottery() = default;

    /**
     * Set new epoch and corresponding randomness, threshold and keypair values
     * @param epoch is an number of epoch where we calculate leadership
     * @param randomness is an epoch random byte sequence
     * @param threshold is a maximum value that is considered valid by vrf
     * @param keypair is a current sassafras sign pair
     * @return true if node validator in epoch, and false otherwise
     */
    virtual bool changeEpoch(EpochNumber epoch,
                             const primitives::BlockInfo &best_block) = 0;

    /**
     * Return lottery current epoch
     */
    virtual EpochNumber getEpoch() const = 0;

    struct SlotLeadership {
      std::optional<EphemeralSeed> erased_seed;
    };

    /**
     * Compute leadership for the slot
     * @param slot is a slot number
     * @return none means the peer was not chosen as a leader
     * for that slot, value contains VRF value and proof
     */
    virtual std::optional<SlotLeadership> getSlotLeadership(
        const primitives::BlockHash &block, SlotNumber slot) const = 0;
  };

}  // namespace kagome::consensus::sassafras
