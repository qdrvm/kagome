/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TICKER_IMPL_HPP
#define KAGOME_TICKER_IMPL_HPP

#include "clock/ticker.hpp"

#include <boost/asio/basic_waitable_timer.hpp>
#include <boost/asio/io_context.hpp>

namespace kagome::clock {
  /**
   * Implementation of ticker over boost::asio::basic_waitable_timer
   * Based on: https://gist.github.com/micfan/680f28bda9f2544423d116e470dd6f39
   */
  class TickerImpl : public Ticker {
   public:
    explicit TickerImpl(std::shared_ptr<boost::asio::io_context> io_context,
                        clock::SystemClock::Duration interval);

    ~TickerImpl() override = default;

    void start(clock::SystemClock::Duration delay) override;

    void stop() override;

    bool isStarted() override;

    void asyncCallRepeatedly(
        std::function<void(const std::error_code &)> h) override;

    void onTick(const boost::system::error_code &ec);

   private:
    bool started_;
    boost::asio::basic_waitable_timer<std::chrono::system_clock> timer_;
    std::function<void(const std::error_code &)> callback_;
    clock::SystemClock::Duration interval_;
  };
}  // namespace kagome::clock

#endif  // KAGOME_TICKER_IMPL_HPP
