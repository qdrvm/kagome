/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <atomic>
#include <memory>

namespace kagome::telemetry {
  /**
   * Counter for "peers" telemetry value.
   * `BlockAnnounceProtocol` writes value.
   * `TelemetryServiceImpl` reads value.
   */
  struct PeerCount {
    PeerCount() : v{0} {}
    std::atomic_size_t v;
  };
}  // namespace kagome::telemetry
