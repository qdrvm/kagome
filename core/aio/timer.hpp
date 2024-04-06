/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <chrono>
#include <functional>
#include <memory>

#include "aio/cancel.hpp"
#include "aio/timer.fwd.hpp"

namespace kagome::aio {
  /**
   * Thread safe async timer.
   */
  class Timer {
   public:
    using Delay = std::chrono::milliseconds;
    using Cb = std::function<void()>;

    virtual ~Timer() = default;

    virtual void timer(Cb, Delay) = 0;

    virtual Cancel timerCancel(Cb, Delay) = 0;

    // libp2p alias
    void schedule(Cb cb, Delay delay) {
      timer(std::move(cb), delay);
    }
    Cancel scheduleWithHandle(Cb cb, Delay delay) {
      return timerCancel(std::move(cb), delay);
    }
  };
}  // namespace kagome::aio
