/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_COMMON_HPP
#define KAGOME_COMMON_HPP

#include <cstdint>
#include <vector>

#include "crypto/crypto_types.hpp"
#include "primitives/common.hpp"

namespace kagome::consensus {
  using BabeSlotNumber = uint64_t;

  using AuthorityWeight = uint64_t;
  struct Authority {
    primitives::AuthorityId id;
    AuthorityWeight babe_weight{};
  };

  /// threshold, which must not be exceeded for the party to be a slot leader
  using Threshold = crypto::VRFValue;

  /// random value, which serves as a seed for VRF slot leadership selection
  using Randomness = crypto::VRFValue;

  /// metainformation about the Babe epoch
  struct Epoch {
    /// the epoch index
    uint64_t epoch_index;
    /// starting slot of the epoch
    BabeSlotNumber start_slot;
    /// duration of the epoch (number of slots it takes)
    BabeSlotNumber duration;
    /// authorities of the epoch with their weights
    std::vector<Authority> authorities;
    /// threshold of the epoch
    Threshold threshold;
    /// randomness of the epoch
    Randomness randomness;
  };
}  // namespace kagome::consensus

#endif  // KAGOME_COMMON_HPP
