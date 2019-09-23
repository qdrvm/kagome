/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "clock/impl/basic_waitable_timer.hpp"

namespace kagome::clock {
  BasicWaitableTimer::BasicWaitableTimer(boost::asio::io_context &io_context)
      : timer_{boost::asio::basic_waitable_timer<std::chrono::system_clock>{
          io_context}} {}

  void BasicWaitableTimer::expiresAt(kagome::clock::SystemClock::TimePoint at) {
    timer_.expires_at(at);
  }

  void BasicWaitableTimer::asyncWait(
      const std::function<void(const std::error_code &)> &h) {
    timer_.async_wait(h);
  }
}  // namespace kagome::clock
