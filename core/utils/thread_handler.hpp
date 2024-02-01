/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "utils/weak_io_context.hpp"
#include "utils/weak_io_context_post.hpp"

namespace kagome {
  class ThreadHandler;

  // defined in thread_poll.hpp
  bool runningInThisThread(const ThreadHandler &self);

  class ThreadHandler final {
    enum struct State : uint32_t { kStopped = 0, kStarted };

   public:
    ThreadHandler(ThreadHandler &&) = delete;
    ThreadHandler(const ThreadHandler &) = delete;

    ThreadHandler &operator=(ThreadHandler &&) = delete;
    ThreadHandler &operator=(const ThreadHandler &) = delete;

    explicit ThreadHandler(WeakIoContext io_context)
        : execution_state_{State::kStopped}, ioc_{std::move(io_context)} {}
    ~ThreadHandler() = default;

    void start() {
      execution_state_.store(State::kStarted);
    }

    void stop() {
      execution_state_.store(State::kStopped);
    }

    template <typename F>
    void execute(F &&func) {
      if (State::kStarted == execution_state_.load(std::memory_order_acquire)) {
        post(ioc_, std::forward<F>(func));
      }
    }

    friend void post(ThreadHandler &self, auto f) {
      return self.execute(std::move(f));
    }

    bool isInCurrentThread() const {
      return runningInThisThread(ioc_);
    }

    friend bool runningInThisThread(const ThreadHandler &self) {
      return self.isInCurrentThread();
    }

   private:
    std::atomic<State> execution_state_;
    WeakIoContext ioc_;
  };

}  // namespace kagome
