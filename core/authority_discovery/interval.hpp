/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "aio/timer.hpp"

namespace kagome::authority_discovery {
  /**
   * Exponentially increasing interval
   *
   * Doubles interval duration on each tick until the configured maximum is
   * reached.
   */
  class ExpIncInterval {
   public:
    ExpIncInterval(std::chrono::milliseconds initial,
                   std::chrono::milliseconds max,
                   aio::TimerPtr scheduler)
        : delay_{initial}, max_{max}, scheduler_{scheduler} {}

    void start(std::function<void()> cb) {
      BOOST_ASSERT(not cb_);
      BOOST_ASSERT(cb);
      cb_ = std::move(cb);
      step();
    }

   private:
    void step() {
      handle_ = scheduler_->scheduleWithHandle(
          [this] {
            cb_();
            delay_ = std::min(delay_ * 2, max_);
            step();
          },
          delay_);
    }

    std::chrono::milliseconds delay_;
    std::chrono::milliseconds max_;
    aio::TimerPtr scheduler_;
    std::function<void()> cb_;
    aio::Cancel handle_;
  };
}  // namespace kagome::authority_discovery
