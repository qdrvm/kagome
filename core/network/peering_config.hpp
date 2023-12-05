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
  };

}  // namespace kagome::network
