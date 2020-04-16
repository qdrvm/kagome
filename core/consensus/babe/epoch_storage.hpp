/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
