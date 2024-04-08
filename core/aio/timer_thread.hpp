/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "aio/timer.hpp"

#include <boost/asio/io_context.hpp>

namespace kagome::aio {
  /**
   * Calls timer callback on specified thread.
   */
  class TimerThread : public Timer {
   public:
    TimerThread(TimerPtr timer, std::weak_ptr<boost::asio::io_context> io)
        : timer_{std::move(timer)}, io_{std::move(io)} {}

    void timer(Cb cb, Delay delay) override {
      auto io = io_.lock();
      timer_->timer(wrap(std::move(cb)), delay);
    }

    Cancel timerCancel(Cb cb, Delay delay) override {
      return timer_->timerCancel(wrap(std::move(cb)), delay);
    }

   private:
    Cb wrap(Cb &&cb) {
      return [weak_io{io_}, cb{std::move(cb)}]() mutable {
        auto io = weak_io.lock();
        if (not io) {
          return;
        }
        io->post(std::move(cb));
      };
    }

    TimerPtr timer_;
    std::weak_ptr<boost::asio::io_context> io_;
  };
}  // namespace kagome::aio
