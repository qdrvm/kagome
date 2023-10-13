/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/buffer.hpp"
#include "primitives/common.hpp"
#include "scale/tie.hpp"

namespace kagome::network {
  /**
   * Request for state to another peer
   */
  struct StateRequest {
    SCALE_TIE(3);

    /// Block header hash.
    primitives::BlockHash hash;

    /// Start from this key.
    /// Multiple keys used for nested state start.
    std::vector<common::Buffer> start{};

    /// if 'true' indicates that response should contain raw key-values, rather
    /// than proof.
    bool no_proof;
  };

}  // namespace kagome::network
