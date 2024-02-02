/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/transport/rpc_io_context.hpp"
#include "utils/thread_pool.hpp"
#include "utils/watchdog.hpp"

namespace kagome::api {
  class RpcThreadPool final : public ThreadPool {
   public:
    RpcThreadPool(std::shared_ptr<Watchdog> watchdog,
                  std::shared_ptr<RpcContext> rpc_context)
        : ThreadPool(std::move(watchdog), "rpc", 1, std::move(rpc_context)) {}
  };
}  // namespace kagome::api
