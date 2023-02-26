/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_AUTHORITY_DISCOVERY_INTERVAL_HPP
#define KAGOME_AUTHORITY_DISCOVERY_INTERVAL_HPP

#include <libp2p/basic/scheduler.hpp>

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
                   std::shared_ptr<libp2p::basic::Scheduler> scheduler)
        : delay_{initial}, max_{max}, scheduler_{scheduler} {}

    void start(std::function<void()> cb) {
      assert(not cb_);
      assert(cb);
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
    std::shared_ptr<libp2p::basic::Scheduler> scheduler_;
    std::function<void()> cb_;
    libp2p::basic::Scheduler::Handle handle_;
  };
}  // namespace kagome::authority_discovery

#endif  // KAGOME_AUTHORITY_DISCOVERY_INTERVAL_HPP
