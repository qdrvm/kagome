/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <optional>

#include "consensus/babe/types/babe_configuration.hpp"
#include "consensus/babe/types/slot_leadership.hpp"
#include "consensus/timeline/types.hpp"
#include "crypto/sr25519_types.hpp"
#include "primitives/common.hpp"

namespace kagome::consensus::babe {
  /// Interface for acquiring leadership information for the current Babe epoch.
  class BabeLottery {
   public:
    virtual ~BabeLottery() = default;

    /// Return lottery current epoch
    virtual EpochNumber getEpoch() const = 0;

    /// Changes epoch
    /// @param epoch epoch that switch to
    /// @param block that epoch data based on
    /// @return true if epoch successfully switched and node validator in epoch
    virtual bool changeEpoch(EpochNumber epoch,
                             const primitives::BlockInfo &best_block) = 0;

    /// Check slot leadership
    /// @param block parent of block for which will produced new one if node is
    /// slot-leader
    /// @param slot for which leadership is checked
    /// @return data needed for slot leadership or none
    virtual std::optional<SlotLeadership> getSlotLeadership(
        const primitives::BlockHash &block, SlotNumber slot) const = 0;
  };
}  // namespace kagome::consensus::babe
