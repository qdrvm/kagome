/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <type_traits>

#include <scale/enum_traits.hpp>

#include "common/outcome_throw.hpp"

namespace kagome::network {
  /**
   * Direction, in which to retrieve the blocks
   */
  enum class Direction : uint8_t {
    /// from child to parent
    ASCENDING = 0,
    /// from parent to canonical child
    DESCENDING = 1
  };

}  // namespace kagome::network

SCALE_DEFINE_ENUM_VALUE_RANGE(kagome::network,
                              Direction,
                              kagome::network::Direction::ASCENDING,
                              kagome::network::Direction::DESCENDING);
