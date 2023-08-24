/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_BABEUTIL
#define KAGOME_CONSENSUS_BABEUTIL

#include "consensus/timeline/types.hpp"
#include "primitives/common.hpp"

namespace kagome::consensus::babe {

  /**
   * Auxiliary class to calculate epoch index by slot number.
   * It needed as seperated class because to exclude mutual dependency
   * blockchain mechanic and block production/validation.
   */
  class BabeUtil {
   public:
    virtual ~BabeUtil() = default;

    /**
     * @returns slot for time
     */
    virtual SlotNumber timeToSlot(TimePoint time) const = 0;

    /**
     * @returns timepoint of start of slot #{@param slot}
     */
    virtual TimePoint slotStartTime(SlotNumber slot) const = 0;

    /**
     * @returns timepoint of finish of slot #{@param slot}
     */
    virtual TimePoint slotFinishTime(SlotNumber slot) const = 0;

    /**
     * @returns epoch descriptor for given parent and slot
     */
    virtual outcome::result<EpochDescriptor> slotToEpochDescriptor(
        const primitives::BlockInfo &parent_info,
        SlotNumber slot_number) const = 0;

    /**
     * @returns epoch number for given parent and slot
     */
    outcome::result<EpochNumber> slotToEpoch(
        const primitives::BlockInfo &parent_info,
        SlotNumber slot_number) const {
      OUTCOME_TRY(epoch, slotToEpochDescriptor(parent_info, slot_number));
      return epoch.epoch_number;
    }
  };

}  // namespace kagome::consensus::babe
#endif  // KAGOME_CONSENSUS_BABEUTIL
