/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TICKER_IMPL_HPP
#define KAGOME_TICKER_IMPL_HPP

#include <boost/asio/basic_waitable_timer.hpp>
#include <boost/asio/io_context.hpp>
#include "clock/ticker.hpp"

namespace kagome::clock {
  /**
   * Implementation of ticker over boost::asio::basic_waitable_timer
   * Based on: https://gist.github.com/micfan/680f28bda9f2544423d116e470dd6f39
   */
  class TickerImpl : public Ticker {
   public:
    explicit TickerImpl(std::shared_ptr<boost::asio::io_context> io_context,
                        uint64_t interval);

    ~TickerImpl() override = default;

    void start(clock::SystemClock::Duration duration) override;

    void stop() override;

    bool isStarted() override;

    void asyncCallRepeatedly(
        std::function<void(const std::error_code &)> h) override;

    void on_tick(const boost::system::error_code &ec);

   private:
    bool callback_set_;
    bool started_;
    boost::asio::basic_waitable_timer<std::chrono::system_clock> timer_;
    std::function<void(const std::error_code &)> callback_;
    uint64_t interval_;
  };
}  // namespace kagome::clock

#endif  // KAGOME_TICKER_IMPL_HPP
