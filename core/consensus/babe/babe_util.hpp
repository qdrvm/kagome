/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_BABEUTIL
#define KAGOME_CONSENSUS_BABEUTIL

#include "consensus/babe/common.hpp"
#include "consensus/babe/types/epoch_descriptor.hpp"
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
    virtual BabeSlotNumber timeToSlot(BabeTimePoint time) const = 0;

    /**
     * @returns timepoint of start of slot #{@param slot}
     */
    virtual BabeTimePoint slotStartTime(BabeSlotNumber slot) const = 0;

    /**
     * @returns timepoint of finish of slot #{@param slot}
     */
    virtual BabeTimePoint slotFinishTime(BabeSlotNumber slot) const = 0;

    /**
     * @returns epoch descriptor for given parent and slot
     */
    virtual outcome::result<EpochDescriptor> slotToEpochDescriptor(
        const primitives::BlockInfo &parent_info,
        BabeSlotNumber slot_number) = 0;

    /**
     * @returns epoch number for given parent and slot
     */
    outcome::result<EpochNumber> slotToEpoch(
        const primitives::BlockInfo &parent_info, BabeSlotNumber slot_number) {
      OUTCOME_TRY(epoch, slotToEpochDescriptor(parent_info, slot_number));
      return epoch.epoch_number;
    }
  };

}  // namespace kagome::consensus::babe
#endif  // KAGOME_CONSENSUS_BABEUTIL
