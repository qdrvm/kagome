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

#ifndef KAGOME_EPOCH_HPP
#define KAGOME_EPOCH_HPP

#include <chrono>
#include <vector>

#include "consensus/babe/common.hpp"
#include "primitives/authority.hpp"

namespace kagome::consensus {
  /**
   * Metainformation about the Babe epoch
   */
  struct Epoch {
    /// the epoch index
    EpochIndex epoch_index{};

    /// starting slot of the epoch; can be non-zero, as the node can join in the
    /// middle of the running epoch
    BabeSlotNumber start_slot{};

    /// duration of the epoch (number of slots it takes)
    BabeSlotNumber epoch_duration{};

    /// authorities of the epoch with their weights
    std::vector<primitives::Authority> authorities;

    /// randomness of the epoch
    Randomness randomness{};

    bool operator==(const Epoch &other) const {
      return epoch_index == other.epoch_index && start_slot == other.start_slot
             && epoch_duration == other.epoch_duration
             && authorities == other.authorities
             && randomness == other.randomness;
    }
    bool operator!=(const Epoch &other) const {
      return !(*this == other);
    }
  };
}  // namespace kagome::consensus

#endif  // KAGOME_EPOCH_HPP
