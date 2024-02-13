/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "utils/thread_pool.hpp"
#include "utils/watchdog.hpp"

namespace kagome::network {
  class BeefyThreadPool final : public ThreadPool {
   public:
    BeefyThreadPool(std::shared_ptr<Watchdog> watchdog)
        : ThreadPool(std::move(watchdog), "beefy", 1, std::nullopt) {}
  };
}  // namespace kagome::consensus::grandpa
