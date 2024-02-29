/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "injector/inject.hpp"
#include "utils/thread_pool.hpp"
#include "utils/watchdog.hpp"

namespace kagome::network {
  class BeefyThreadPool final : public ThreadPool {
   public:
    BeefyThreadPool(std::shared_ptr<Watchdog> watchdog, Inject)
        : ThreadPool(std::move(watchdog), "beefy", 1, std::nullopt) {}

    // Ctor for test purposes
    BeefyThreadPool(TestThreadPool test) : ThreadPool{test} {}
  };
}  // namespace kagome::network
