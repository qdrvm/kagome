/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "utils/thread_pool.hpp"
#include "utils/watchdog.hpp"

namespace kagome::consensus::grandpa {
  class GrandpaThreadPool final : public ThreadPool {
   public:
    GrandpaThreadPool(std::shared_ptr<Watchdog> watchdog)
        : ThreadPool(std::move(watchdog), "grandpa", 1, std::nullopt) {}
  };
}  // namespace kagome::consensus::grandpa
