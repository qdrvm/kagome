/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <deque>
#include <functional>
#include <libp2p/common/final_action.hpp>
#include <mutex>

#include "utils/weak_io_context_post.hpp"

namespace kagome {
  class WeakIoContextStrand
      : public std::enable_shared_from_this<WeakIoContextStrand> {
   public:
    WeakIoContextStrand(WeakIoContext io) : io_{std::move(io)} {}

    void post(auto f) {
      kagome::post(io_, [weak{weak_from_this()}, f{std::move(f)}]() mutable {
        if (auto self = weak.lock()) {
          self->work(std::move(f));
        }
      });
    }

   private:
    void work(auto &&f) {
      std::unique_lock lock{mutex_};
      if (running_) {
        queue_.emplace_back(std::move(f));
        return;
      }
      running_ = true;
      libp2p::common::FinalAction BOOST_OUTCOME_TRY_UNIQUE_NAME{[&] {
        if (not lock.owns_lock()) {
          lock.lock();
        }
        running_ = false;
      }};
      lock.unlock();
      f();
      lock.lock();
      while (not queue_.empty()) {
        auto f = std::move(queue_.front());
        queue_.pop_front();
        lock.unlock();
        f();
        lock.lock();
      }
    }

    WeakIoContext io_;
    std::mutex mutex_;
    bool running_ = false;
    std::deque<std::function<void()>> queue_;
  };
}  // namespace kagome
