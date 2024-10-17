/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <vector>
#include <cstdint>

namespace kagome::network {

  /**
   * Response to the StateRequest but compressed
   */
  struct StateResponseCompressed {
    /// Compressed state Response
    std::vector<uint8_t> data;
  };
}  // namespace kagome::network
