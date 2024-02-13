/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "utils/thread_pool.hpp"
#include "utils/watchdog.hpp"

namespace kagome::parachain {
  class ApprovalThreadPool final : public ThreadPool {
   public:
    ApprovalThreadPool(std::shared_ptr<Watchdog> watchdog)
        : ThreadPool(std::move(watchdog), "approval", 1, std::nullopt) {}
  };
}  // namespace kagome::parachain
