/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>

namespace kagome::consensus::babe {

  enum class SlotType : uint8_t {
    /// A primary VRF-based slot assignment
    Primary = 1,
    /// A secondary deterministic slot assignment
    SecondaryPlain = 2,
    /// A secondary deterministic slot assignment with VRF outputs
    SecondaryVRF = 3,
  };

  inline std::string_view to_string(SlotType s) {
    switch (s) {
      case SlotType::Primary:
        return "Primary";
      case SlotType::SecondaryPlain:
        return "Secondary Plain";
      case SlotType::SecondaryVRF:
        return "Secondary VRF";
    }
    return "Unknown";
  }

}  // namespace kagome::consensus::babe
