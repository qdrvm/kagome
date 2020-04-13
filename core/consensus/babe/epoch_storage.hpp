/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_EPOCH_STORAGE_HPP
#define KAGOME_EPOCH_STORAGE_HPP

#include <boost/optional.hpp>

#include "consensus/babe/common.hpp"
#include "consensus/babe/types/next_epoch_descriptor.hpp"

namespace kagome::consensus {
  /**
   * Allows to store epochs
   */
  struct EpochStorage {
    virtual ~EpochStorage() = default;

    /**
     * Stores epoch's information by its number
     * @param epoch_number number of stored epoch
     * @param epoch_descriptor epochs information
     * @return result with success if epoch was added, error otherwise
     */
    virtual outcome::result<void> addEpochDescriptor(
        EpochIndex epoch_number,
        const NextEpochDescriptor &epoch_descriptor) = 0;

    /**
     * Get an epoch by a (\param block_id)
     * @return epoch or nothing, if epoch, in which that block was produced, is
     * unknown to this peer
     */
    virtual outcome::result<NextEpochDescriptor> getEpochDescriptor(
        EpochIndex epoch_number) const = 0;
  };
}  // namespace kagome::consensus

#endif  // KAGOME_EPOCH_STORAGE_HPP
