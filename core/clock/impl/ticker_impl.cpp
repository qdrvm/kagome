/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "clock/impl/ticker_impl.hpp"

#include <boost/bind.hpp>

namespace kagome::clock {
  TickerImpl::TickerImpl(std::shared_ptr<boost::asio::io_context> io_context,
                         uint64_t interval)
      : callback_set_{false},
        started_{false},
        timer_{boost::asio::basic_waitable_timer<std::chrono::system_clock>{
            *io_context}},
        interval_{interval} {}

  void TickerImpl::start(uint64_t msec) {
    if (callback_set_) {
      started_ = true;
      timer_.expires_from_now(boost::asio::chrono::milliseconds(msec));
      timer_.async_wait(
          [&](const boost::system::error_code &ec) { on_tick(ec); });
    }
  }

  void TickerImpl::stop() {
    started_ = false;
    timer_.cancel();
  }

  bool TickerImpl::isStarted() {
    return started_;
  }

  void TickerImpl::asyncCallRepeatedly(
      std::function<void(const std::error_code &)> h) {
    callback_ = std::move(h);
    callback_set_ = true;
  }

  void TickerImpl::on_tick(const boost::system::error_code &ec) {
    callback_(ec);
    timer_.expires_from_now(boost::asio::chrono::milliseconds(interval_));
    timer_.async_wait(boost::bind(&TickerImpl::on_tick, this, ec));
  }
}  // namespace kagome::clock
