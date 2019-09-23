/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TIMER_HPP
#define KAGOME_TIMER_HPP

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
    virtual void expiresAt(kagome::clock::SystemClock::TimePoint at) = 0;

    /**
     * Wait for the timer expiration
     * @param h - handler, which is called, when the timer is expired, or error
     * happens
     */
    virtual void asyncWait(
        const std::function<void(const std::error_code &)> &h) = 0;
  };
}  // namespace kagome::clock

#endif  // KAGOME_TIMER_HPP
