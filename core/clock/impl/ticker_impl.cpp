/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "clock/impl/ticker_impl.hpp"

#include <boost/bind.hpp>

namespace kagome::clock {
  TickerImpl::TickerImpl(std::shared_ptr<boost::asio::io_context> io_context,
                         clock::SystemClock::Duration interval)
      : started_{false},
        timer_{boost::asio::basic_waitable_timer<std::chrono::system_clock>{
            *io_context}},
        interval_{interval} {}

  void TickerImpl::start(clock::SystemClock::Duration duration) {
    if (callback_) {
      started_ = true;
      timer_.expires_after(duration);
      timer_.async_wait(
          [&](const boost::system::error_code &ec) { onTick(ec); });
    }
  }

  void TickerImpl::stop() {
    started_ = false;
    timer_.cancel();
  }

  bool TickerImpl::isStarted() {
    return started_;
  }

  clock::SystemClock::Duration TickerImpl::interval() {
    return interval_;
  }

  void TickerImpl::asyncCallRepeatedly(
      std::function<void(const std::error_code &)> h) {
    callback_ = std::move(h);
  }

  void TickerImpl::onTick(const boost::system::error_code &ec) {
    callback_(ec);
    if (started_) {
      timer_.expires_after(interval_);
      timer_.async_wait(boost::bind(&TickerImpl::onTick, this, ec));
    }
  }
}  // namespace kagome::clock
