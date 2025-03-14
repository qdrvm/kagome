/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <optional>

#include "common/buffer.hpp"

namespace kagome::primitives {

  /**
   * Index of an extrinsic in a block
   */
  using ExtrinsicIndex = uint32_t;

  /**
   * @brief Extrinsic class represents extrinsic
   */
  struct Extrinsic {
    common::Buffer data;  ///< extrinsic content as byte array
    bool operator==(const Extrinsic &other) const = default;
  };
}  // namespace kagome::primitives
