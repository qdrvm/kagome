/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_PEERING_CONFIG
#define KAGOME_NETWORK_PEERING_CONFIG

#include <chrono>

namespace kagome::network {

  struct PeeringConfig {
    /// Time of peer inactivity to disconnect
    std::chrono::seconds peerTtl = std::chrono::minutes(10);

    /// Period of active peers amount aligning
    std::chrono::seconds aligningPeriod = std::chrono::seconds(5);

    /// Target amount of active peers
#ifdef NDEBUG
    size_t targetPeerAmount = 20;
#else
    size_t targetPeerAmount = 20;
#endif

    /// Max peers count before start to disconnect of innactive ones
    size_t softLimit = 40;

    /// Max peers before start forced disconnection
    size_t hardLimit = 60;
  };

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_PEERING_CONFIG
