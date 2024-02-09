/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

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
}  // namespace kagome::common
