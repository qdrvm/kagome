/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "utils/thread_pool.hpp"
#include "utils/watchdog.hpp"

namespace kagome::telemetry {
  class TelemetryThreadPool : public ThreadPool {
   public:
    TelemetryThreadPool(std::shared_ptr<Watchdog> watchdog)
        : ThreadPool(std::move(watchdog), "telemetry", 1, std::nullopt) {}
  };
}  // namespace kagome::telemetry
