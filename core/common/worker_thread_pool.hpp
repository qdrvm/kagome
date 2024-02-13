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
    WorkerThreadPool(Inject, std::shared_ptr<Watchdog> watchdog)
        : ThreadPool(
            std::move(watchdog),
            "worker",
            std::max<size_t>(3, std::thread::hardware_concurrency()) - 1,
            std::nullopt) {}

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
