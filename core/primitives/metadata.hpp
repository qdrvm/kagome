/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <vector>

namespace kagome::primitives {

  /**
   * Polkadot primitive, which is opaque representation of RuntimeMetadata
   */
  using OpaqueMetadata = std::vector<uint8_t>;

}  // namespace kagome::primitives
