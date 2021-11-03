/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_NETWORK_TYPES_BLOCK_DIRECTION_HPP
#define KAGOME_CORE_NETWORK_TYPES_BLOCK_DIRECTION_HPP

#include <cstdint>
#include <type_traits>

#include "common/outcome_throw.hpp"
#include "scale/scale_error.hpp"

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

#endif  // KAGOME_CORE_NETWORK_TYPES_BLOCK_DIRECTION_HPP
