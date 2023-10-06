/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <functional>
#include <system_error>

#include "clock/clock.hpp"

namespace kagome::clock {
  /**
   * Interface for asynchronous timer
   */
  struct Timer {
    virtual ~Timer() = default;

    /**
     * Set an expire time for this timer
     * @param at - timepoint, at which the timer expires
     */
    virtual void expiresAt(clock::SystemClock::TimePoint at) = 0;

    /**
     * Set an expire time for this timer
     * @param duration before timer will be expired
     */
    virtual void expiresAfter(clock::SystemClock::Duration duration) = 0;

    /**
     * Cancel timer
     */
    virtual void cancel() = 0;

    /**
     * Wait for the timer expiration
     * @param h - handler, which is called, when the timer is expired, or error
     * happens
     */
    virtual void asyncWait(
        const std::function<void(const boost::system::error_code &)> &h) = 0;
  };
}  // namespace kagome::clock
