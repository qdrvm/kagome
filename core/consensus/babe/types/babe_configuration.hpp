/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/blob.hpp"
#include "consensus/babe/types/authority.hpp"
#include "consensus/timeline/types.hpp"

namespace kagome::consensus::babe {

  enum class AllowedSlots : uint8_t {
    PrimaryOnly,
    PrimaryAndSecondaryPlain,
    PrimaryAndSecondaryVRF
  };

  inline std::string_view to_string(AllowedSlots s) {
    switch (s) {
      case AllowedSlots::PrimaryOnly:
        return "Primary only";
      case AllowedSlots::PrimaryAndSecondaryPlain:
        return "Primary and Secondary Plain";
      case AllowedSlots::PrimaryAndSecondaryVRF:
        return "Primary and Secondary VRF";
    }
    return "Unknown";
  }

  /// Configuration data used by the BABE consensus engine.
  struct BabeConfiguration {
    /// The slot duration in milliseconds for BABE. Currently, only
    /// the value provided by this type at genesis will be used.
    ///
    /// Dynamic slot duration may be supported in the future.
    SlotDuration slot_duration{};  // must be permanent

    SlotNumber epoch_length{};  // must be permanent

    /// A constant value that is used in the threshold calculation formula.
    /// Expressed as a rational where the first member of the tuple is the
    /// numerator and the second is the denominator. The rational should
    /// represent a value between 0 and 1.
    /// In substrate it is called `c`
    /// In the threshold formula calculation, `1 - leadership_rate` represents
    /// the probability of a slot being empty.
    std::pair<uint64_t, uint64_t> leadership_rate;  // changes by NextConfigData

    /// The authorities for block production
    Authorities authorities;  // can be changed by NextEpochData & OnDisabled

    /// The randomness for the genesis epoch.
    Randomness randomness;  // can be changed by NextEpochData

    /// Type of allowed slots.
    AllowedSlots allowed_slots{};  // can be changed by NextConfigData
  };

  struct Epoch {
    EpochNumber epoch_index;
    SlotNumber start_slot;
    EpochLength duration;
    Authorities authorities;
    Randomness randomness;
    std::pair<uint64_t, uint64_t> leadership_rate;
    AllowedSlots allowed_slots;
  };

}  // namespace kagome::consensus::babe
