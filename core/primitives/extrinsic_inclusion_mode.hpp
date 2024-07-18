/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>

namespace kagome {
  enum class ExtrinsicInclusionMode : uint8_t {
    AllExtrinsics,
    OnlyInherents,
  };
}  // namespace kagome
