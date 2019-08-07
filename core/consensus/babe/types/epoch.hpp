/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_EPOCH_HPP
#define KAGOME_EPOCH_HPP

#include <chrono>
#include <vector>

#include "consensus/babe/common.hpp"
#include "consensus/babe/types/authority.hpp"

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

    /// duration of the slot
    std::chrono::milliseconds slot_duration{};

    /// authorities of the epoch with their weights
    std::vector<Authority> authorities;

    /// threshold of the epoch
    Threshold threshold;

    /// randomness of the epoch
    Randomness randomness{};
  };
}  // namespace kagome::consensus

#endif  // KAGOME_EPOCH_HPP
