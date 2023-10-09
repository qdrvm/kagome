/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "clock/impl/basic_waitable_timer.hpp"

#include <boost/system/error_code.hpp>

namespace kagome::clock {
  BasicWaitableTimer::BasicWaitableTimer(
      std::shared_ptr<boost::asio::io_context> io_context)
      : io_context_([&] {
          BOOST_ASSERT(io_context);
          return std::move(io_context);
        }()),
        timer_{boost::asio::basic_waitable_timer<std::chrono::system_clock>{
            *io_context_}} {}

  void BasicWaitableTimer::expiresAt(clock::SystemClock::TimePoint at) {
    timer_.expires_at(at);
  }

  void BasicWaitableTimer::expiresAfter(clock::SystemClock::Duration duration) {
    timer_.expires_after(duration);
  }

  void BasicWaitableTimer::cancel() {
    timer_.cancel();
  }

  void BasicWaitableTimer::asyncWait(
      const std::function<void(const boost::system::error_code &)> &h) {
    timer_.async_wait(h);
  }
}  // namespace kagome::clock
