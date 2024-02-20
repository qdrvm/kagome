/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "application/app_state_manager.hpp"
#include "injector/inject.hpp"
#include "utils/thread_pool.hpp"
#include "utils/watchdog.hpp"

namespace kagome::common {
  class WorkerThreadPool final : public ThreadPool {
   public:
    WorkerThreadPool(std::shared_ptr<Watchdog> watchdog, size_t thread_number)
        : ThreadPool(
            std::move(watchdog), "worker", thread_number, std::nullopt) {}

    WorkerThreadPool(std::shared_ptr<Watchdog> watchdog, Inject, ...)
        : WorkerThreadPool(
            std::move(watchdog),
            std::max<size_t>(3, std::thread::hardware_concurrency()) - 1) {}

    // Ctor for test purposes
    WorkerThreadPool(TestThreadPool test) : ThreadPool{test} {}
  };

  class WorkerPoolHandler final : public PoolHandler {
   public:
    WorkerPoolHandler(
        std::shared_ptr<application::AppStateManager> app_state_manager,
        std::shared_ptr<WorkerThreadPool> thread_pool)
        : PoolHandler(thread_pool->io_context()) {
      BOOST_ASSERT(app_state_manager);
      app_state_manager->takeControl(*this);
    }
  };
}  // namespace kagome::common
