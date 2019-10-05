/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
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
