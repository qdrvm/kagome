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
    using T = std::atomic_size_t;
    PeerCount() : v{std::make_shared<T>()} {}
    std::shared_ptr<T> v;
  };
}  // namespace kagome::telemetry
