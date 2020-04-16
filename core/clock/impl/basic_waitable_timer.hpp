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

#ifndef KAGOME_BASIC_WAITABLE_TIMER_HPP
#define KAGOME_BASIC_WAITABLE_TIMER_HPP

#include <boost/asio/basic_waitable_timer.hpp>
#include <boost/asio/io_context.hpp>
#include "clock/timer.hpp"

namespace kagome::clock {
  /**
   * Implementation of timer over boost::asio::basic_waitable_timer
   */
  class BasicWaitableTimer : public Timer {
   public:
    explicit BasicWaitableTimer(boost::asio::io_context &io_context);

    ~BasicWaitableTimer() override = default;

    void expiresAt(clock::SystemClock::TimePoint at) override;

    void asyncWait(
        const std::function<void(const std::error_code &)> &h) override;

   private:
    boost::asio::basic_waitable_timer<std::chrono::system_clock> timer_;
  };
}  // namespace kagome::clock

#endif  // KAGOME_BASIC_WAITABLE_TIMER_HPP
