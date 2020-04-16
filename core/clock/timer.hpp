/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
    virtual void expiresAt(clock::SystemClock::TimePoint at) = 0;

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
