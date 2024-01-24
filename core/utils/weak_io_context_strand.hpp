/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <deque>
#include <functional>
#include <libp2p/common/final_action.hpp>

#include "utils/safe_object.hpp"
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
      if (SAFE_UNIQUE(state_) {
            if (state_.running_) {
              state_.queue_.emplace_back(std::move(f));
              return true;
            }
            state_.running_ = true;
            return false;
          }) {
        return;
      }
      libp2p::common::FinalAction BOOST_OUTCOME_TRY_UNIQUE_NAME{[&] {
        SAFE_UNIQUE(state_) {
          state_.running_ = false;
        };
      }};
      f();
      while (auto f = SAFE_UNIQUE(state_) {
        std::optional<std::function<void()>> f;
        if (not state_.queue_.empty()) {
          f = std::move(state_.queue_.front());
          state_.queue_.pop_front();
        }
        return f;
      }) {
        (*f)();
      }
    }

    WeakIoContext io_;
    struct State {
      bool running_ = false;
      std::deque<std::function<void()>> queue_;
    };
    SafeObject<State, std::mutex> state_;
  };
}  // namespace kagome
