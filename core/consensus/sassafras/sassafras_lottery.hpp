/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <optional>

#include "consensus/sassafras/types/randomness.hpp"
#include "consensus/sassafras/types/slot_leadership.hpp"
#include "consensus/sassafras/types/ticket.hpp"
#include "consensus/timeline/types.hpp"
#include "crypto/bandersnatch_types.hpp"
#include "primitives/common.hpp"

namespace kagome::consensus::sassafras {

  class SassafrasLottery {
   public:
    virtual ~SassafrasLottery() = default;

    /// Return lottery current epoch
    virtual EpochNumber getEpoch() const = 0;

    /// Changes epoch
    /// @param epoch epoch that switch to
    /// @param block that epoch data based on
    /// @return true if epoch successfully switched and node validator in epoch
    virtual bool changeEpoch(EpochNumber epoch,
                             const primitives::BlockInfo &block) = 0;

    /// Check slot leadership
    /// @param block parent of block for which will produced new one if node is
    /// slot-leader
    /// @param slot for which leadership is checked
    /// @return data needed for slot leadership or none
    virtual std::optional<SlotLeadership> getSlotLeadership(
        const primitives::BlockHash &block, SlotNumber slot) const = 0;
  };

}  // namespace kagome::consensus::sassafras
