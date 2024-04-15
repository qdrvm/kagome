/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "utils/thread_pool.hpp"
#include "utils/watchdog.hpp"

namespace kagome::parachain {
  class PvfThreadPool final : public ThreadPool {
   public:
    PvfThreadPool(std::shared_ptr<Watchdog> watchdog, Inject, ...)
        : ThreadPool(std::move(watchdog), "pvf", 1, std::nullopt) {}

    PvfThreadPool(TestThreadPool test) : ThreadPool{test} {}
  };
}  // namespace kagome::parachain
