/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/timeline/types.hpp"
#include "primitives/common.hpp"

namespace kagome::consensus {

  /**
   * Auxiliary class to calculate epoch index by slot number.
   * It needed as seperated class because to exclude mutual dependency
   * blockchain mechanic and block production/validation.
   */
  class SlotsUtil {
   public:
    SlotsUtil() = default;
    SlotsUtil(SlotsUtil &&) = delete;
    SlotsUtil(const SlotsUtil &) = delete;

    virtual ~SlotsUtil() = default;

    SlotsUtil &operator=(SlotsUtil &&) = delete;
    SlotsUtil &operator=(const SlotsUtil &) = delete;

    /// @return the duration of a slot in milliseconds
    virtual Duration slotDuration() const = 0;

    /// @return the epoch length in slots
    virtual EpochLength epochLength() const = 0;

    /// @returns slot for time
    virtual SlotNumber timeToSlot(TimePoint time) const = 0;

    /// @returns timepoint of start of slot #{@param slot}
    virtual TimePoint slotStartTime(SlotNumber slot) const = 0;

    /// @returns timepoint of finish of slot #{@param slot}
    virtual TimePoint slotFinishTime(SlotNumber slot) const = 0;

    /// @returns epoch number for given parent and slot
    /// @note virtual - for being mocked
    virtual outcome::result<EpochNumber> slotToEpoch(
        const primitives::BlockInfo &parent_info,
        SlotNumber slot_number) const = 0;
  };

}  // namespace kagome::consensus
