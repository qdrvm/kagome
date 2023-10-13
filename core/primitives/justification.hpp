/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/buffer.hpp"
#include "scale/tie.hpp"

namespace kagome::primitives {
  /**
   * Justification of the finalized block
   */
  struct Justification {
    SCALE_TIE(1);

    common::Buffer data;
  };
}  // namespace kagome::primitives
