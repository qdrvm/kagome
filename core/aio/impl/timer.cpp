/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "aio/impl/timer.hpp"

#include <boost/asio/io_context.hpp>

namespace kagome::aio {
  TimerImplCancel::State::~State() {
    SAFE_UNIQUE(handle) {
      handle.reset();
    };
  }

  TimerImplCancel::TimerImplCancel(std::weak_ptr<boost::asio::io_context> io,
                                   std::shared_ptr<State> state)
      : io_{std::move(io)}, state_{std::move(state)} {}

  TimerImplCancel::~TimerImplCancel() {
    state_->cancelled.store(true);
    auto io = io_.lock();
    if (not io) {
      return;
    }
    io->post([state{std::move(state_)}]() mutable { state.reset(); });
  }

  TimerImpl::TimerImpl(std::weak_ptr<boost::asio::io_context> io,
                       std::weak_ptr<libp2p::basic::Scheduler> scheduler)
      : io_{std::move(io)}, scheduler_{std::move(scheduler)} {}

  void TimerImpl::timer(Cb cb, Delay delay) {
    auto io = io_.lock();
    if (not io) {
      return;
    }
    io->post([weak_scheduler{scheduler_}, cb{std::move(cb)}, delay]() mutable {
      auto scheduler = weak_scheduler.lock();
      if (not scheduler) {
        return;
      }
      scheduler->schedule(std::move(cb), delay);
    });
  }

  Cancel TimerImpl::timerCancel(Cb cb, Delay delay) {
    auto io = io_.lock();
    if (not io) {
      return Cancel{};
    }
    auto state = std::make_shared<TimerImplCancel::State>();
    io->post([weak_scheduler{scheduler_},
              cb{std::move(cb)},
              delay,
              weak_state{std::weak_ptr{state}}]() mutable {
      auto state = weak_state.lock();
      if (not state) {
        return;
      }
      if (state->cancelled.load()) {
        return;
      }
      auto scheduler = weak_scheduler.lock();
      if (not scheduler) {
        return;
      }
      cb = [cb{std::move(cb)}, weak_state]() mutable {
        auto state = weak_state.lock();
        if (not state) {
          return;
        }
        if (state->cancelled.load()) {
          return;
        }
        cb();
      };
      auto handle = scheduler->scheduleWithHandle(std::move(cb), delay);
      auto &handle_out = state->handle;
      SAFE_UNIQUE(handle_out) {
        handle_out = std::move(handle);
      };
    });
    return std::make_unique<TimerImplCancel>(io_, std::move(state));
  }
}  // namespace kagome::aio
