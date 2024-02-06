/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "utils/thread_pool.hpp"
#include "utils/watchdog.hpp"

namespace kagome::common {
  class WorkerThreadPool final : public ThreadPool {
   public:
    WorkerThreadPool(std::shared_ptr<Watchdog> watchdog)
        : ThreadPool(
            std::move(watchdog),
            "worker",
            std::max<size_t>(3, std::thread::hardware_concurrency()) - 1,
            std::nullopt) {}
  };
}  // namespace kagome::common
