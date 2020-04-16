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

#include "clock/impl/basic_waitable_timer.hpp"

namespace kagome::clock {
  BasicWaitableTimer::BasicWaitableTimer(boost::asio::io_context &io_context)
      : timer_{boost::asio::basic_waitable_timer<std::chrono::system_clock>{
          io_context}} {}

  void BasicWaitableTimer::expiresAt(clock::SystemClock::TimePoint at) {
    timer_.expires_at(at);
  }

  void BasicWaitableTimer::asyncWait(
      const std::function<void(const std::error_code &)> &h) {
    timer_.async_wait(h);
  }
}  // namespace kagome::clock
