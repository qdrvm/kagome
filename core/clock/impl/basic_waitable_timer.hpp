/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/asio/basic_waitable_timer.hpp>
#include <boost/asio/io_context.hpp>
#include "clock/timer.hpp"

namespace kagome::clock {
  /**
   * Implementation of timer over boost::asio::basic_waitable_timer
   */
  class BasicWaitableTimer : public Timer {
   public:
    explicit BasicWaitableTimer(
        std::shared_ptr<boost::asio::io_context> io_context);

    ~BasicWaitableTimer() override = default;

    void expiresAt(clock::SystemClock::TimePoint at) override;

    void expiresAfter(clock::SystemClock::Duration duration) override;

    void cancel() override;

    void asyncWait(const std::function<void(const boost::system::error_code &)>
                       &h) override;

   private:
    std::shared_ptr<boost::asio::io_context> io_context_;
    boost::asio::basic_waitable_timer<std::chrono::system_clock> timer_;
  };
}  // namespace kagome::clock
