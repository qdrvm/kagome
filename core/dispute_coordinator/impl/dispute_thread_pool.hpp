/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "utils/thread_pool.hpp"
#include "utils/watchdog.hpp"

namespace kagome::dispute {
  class DisputeThreadPool final : public ThreadPool {
   public:
    DisputeThreadPool(std::shared_ptr<Watchdog> watchdog)
        : ThreadPool(std::move(watchdog), "dispute", 1, std::nullopt) {}
  };
}  // namespace kagome::dispute
