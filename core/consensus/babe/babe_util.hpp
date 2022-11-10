/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_BABEUTIL
#define KAGOME_CONSENSUS_BABEUTIL

#include "consensus/babe/common.hpp"
#include "consensus/babe/types/epoch_descriptor.hpp"

namespace kagome::consensus {

  /**
   * Auxiliary class to calculate epoch index by slot number.
   * It needed as seperated class because to exclude mutual dependency
   * blockchain mechanic and block production/validation.
   */
  class BabeUtil {
   public:
    virtual ~BabeUtil() = default;

    /**
     * Init inner state by call {@param f} returning first block slot and flag
     * if first block is already finalized
     */
    virtual BabeSlotNumber syncEpoch(
        std::function<std::tuple<BabeSlotNumber, bool>()> &&f) = 0;

    /**
     * @returns current unix time slot number
     */
    virtual BabeSlotNumber getCurrentSlot() const = 0;

    /**
     * @returns timepoint of start of slot #{@param slot}
     */
    virtual BabeTimePoint slotStartTime(BabeSlotNumber slot) const = 0;

    /**
     * @returns duration to start of slot #{@param slot}
     */
    virtual BabeDuration remainToStartOfSlot(BabeSlotNumber slot) const = 0;

    /**
     * @returns timepoint of finish of slot #{@param slot}
     */
    virtual BabeTimePoint slotFinishTime(BabeSlotNumber slot) const = 0;

    /**
     * @returns duration to finish of slot #{@param slot}
     */
    virtual BabeDuration remainToFinishOfSlot(BabeSlotNumber slot) const = 0;

    /**
     * @returns number of epoch by provided {@param slot_number}
     */
    virtual EpochNumber slotToEpoch(BabeSlotNumber slot_number) const = 0;

    /**
     * @returns ordinal number of the slot in the corresponding epoch by
     * provided {@param slot_number}
     */
    virtual BabeSlotNumber slotInEpoch(BabeSlotNumber slot_number) const = 0;
  };

}  // namespace kagome::consensus
#endif  // KAGOME_CONSENSUS_BABEUTIL
