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
     * @returns current unix time slot number
     */
    virtual BabeSlotNumber getCurrentSlot() const = 0;

    /**
     * @returns estimate of time to start slot
     */
    virtual BabeDuration slotStartsIn(BabeSlotNumber slot) const = 0;

    /**
     * @returns configured slot duration
     */
    virtual BabeDuration slotDuration() const = 0;

    /**
     * @returns number of epoch by provided {@param slot_number}
     */
    virtual EpochNumber slotToEpoch(BabeSlotNumber slot_number) const = 0;

    /**
     * @returns ordinal number of the slot in the corresponding epoch by
     * provided {@param slot_number}
     */
    virtual BabeSlotNumber slotInEpoch(BabeSlotNumber slot_number) const = 0;

    /**
     * Stores epoch's data for last active epoch
     * @param led LastEpochDescriptor of last active epoch
     * @return result of store
     */
    virtual outcome::result<void> setLastEpoch(const EpochDescriptor &led) = 0;

    /**
     * Get number of last active epoch
     * @return number of epoch that stored as last active, error otherwise
     */
    virtual outcome::result<EpochDescriptor> getLastEpoch() const = 0;
  };

}  // namespace kagome::consensus
#endif  // KAGOME_CONSENSUS_BABEUTIL
