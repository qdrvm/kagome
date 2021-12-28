/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SLOT_HPP
#define KAGOME_SLOT_HPP

#include <cstdint>

namespace kagome::consensus {

  enum class SlotType : uint8_t {
    /// A primary VRF-based slot assignment
    Primary = 1,
    /// A secondary deterministic slot assignment
    SecondaryPlain = 2,
    /// A secondary deterministic slot assignment with VRF outputs
    SecondaryVRF = 3,
  };

}

#endif  // KAGOME_SLOT_HPP
