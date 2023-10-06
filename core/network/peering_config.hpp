/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <chrono>

namespace kagome::network {

  struct PeeringConfig {
    /// Time of peer inactivity to disconnect
    std::chrono::seconds peerTtl = std::chrono::minutes(10);

    /// Period of active peers amount aligning
    std::chrono::seconds aligningPeriod = std::chrono::seconds(5);

    /// Target amount of active peers
    size_t targetPeerAmount = 20;

    /// Max peers count before start to disconnect of inactive ones
    size_t softLimit = 40;

    /// Max peers before start forced disconnection
    size_t hardLimit = 60;
  };

}  // namespace kagome::network
