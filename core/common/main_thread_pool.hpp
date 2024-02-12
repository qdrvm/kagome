/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "application/app_state_manager.hpp"
#include "utils/thread_pool.hpp"
#include "utils/watchdog.hpp"

#include <boost/asio/io_context.hpp>

namespace kagome::common {
  class MainThreadPool final : public ThreadPool {
   public:
    MainThreadPool(std::shared_ptr<Watchdog> watchdog,
                   std::shared_ptr<boost::asio::io_context> ctx)
        : ThreadPool(std::move(watchdog), "main_runner", 1, std::move(ctx)) {}
  };

  class MainPoolHandler final : public ThreadHandler {
   public:
    MainPoolHandler(
        std::shared_ptr<application::AppStateManager> app_state_manager,
        std::shared_ptr<MainThreadPool> thread_pool)
        : ThreadHandler(thread_pool->io_context()) {
      BOOST_ASSERT(app_state_manager);
      app_state_manager->takeControl(*this);
    }
  };
}  // namespace kagome::common
