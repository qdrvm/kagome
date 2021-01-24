/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_EPOCHSTORAGE
#define KAGOME_CONSENSUS_EPOCHSTORAGE

#include <boost/optional.hpp>

#include "consensus/babe/common.hpp"
#include "consensus/babe/types/last_epoch_descriptor.hpp"
#include "consensus/babe/types/next_epoch_descriptor.hpp"
#include "primitives/common.hpp"

namespace kagome::consensus {
  /**
   * Allows to store epochs
   */
  struct EpochStorage {
    virtual ~EpochStorage() = default;

    /**
     * Stores epoch's data for last active epoch
     * @param led LastEpochDescriptor of last active epoch
     * @return result of store
     */
    virtual outcome::result<void> setLastEpoch(
        const LastEpochDescriptor &led) = 0;

    /**
     * Get number of last active epoch
     * @return number of epoch that stored as last active, error otherwise
     */
    virtual outcome::result<LastEpochDescriptor> getLastEpoch() const = 0;
  };
}  // namespace kagome::consensus

#endif  // KAGOME_CONSENSUS_EPOCHSTORAGE
