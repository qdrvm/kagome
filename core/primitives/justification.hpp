/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/buffer.hpp"

namespace kagome::primitives {
  /**
   * Justification of the finalized block
   */
  struct Justification {
    common::Buffer data;
    bool operator==(const Justification &other) const = default;
  };
}  // namespace kagome::primitives
