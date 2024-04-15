/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <deque>

#include "utils/pool_handler.hpp"
#include "utils/safe_object.hpp"

namespace kagome {
  /**
   * Enqueue `post` callbacks to run after `setReady`.
   * Use `postAlways` for initialization code.
   * Call `setReady` after initialization.
   *
   * Example:
   *   struct Component {
   *     PoolHandlerReady thread;
   *     void init() {
   *       thread.postAlways([this] {
   *         // initialize `Component`
   *         ...
   *         // mark thread as ready, call pending `post` callbacks
   *         thread.setReady();
   *       });
   *     }
   *     void action() {
   *       // `post` will enqueue callback until `thread.setReady` call
   *       REINVOKE(action);
   *       // `thread.setReady` was called, `Component` is initialized
   *       ...
   *     }
   *   }
   */
  class PoolHandlerReady {
   public:
    PoolHandlerReady(PoolHandlerReady &&) = delete;
    PoolHandlerReady(const PoolHandlerReady &) = delete;

    PoolHandlerReady &operator=(PoolHandlerReady &&) = delete;
    PoolHandlerReady &operator=(const PoolHandlerReady &) = delete;

    DONT_INJECT(PoolHandlerReady);

    explicit PoolHandlerReady(std::shared_ptr<boost::asio::io_context> io)
        : io_{std::move(io)} {}

    void setReady() {
      SAFE_UNIQUE(pending_) {
        if (pending_) {
          auto pending = std::move(*pending_);
          pending_.reset();
          if (not stopped_.test()) {
            for (auto &f : pending) {
              io_->post(std::move(f));
            }
          }
        }
      };
    }

    void stop() {
      if (stopped_.test_and_set()) {
        throw std::logic_error{"already stopped"};
      }
    }

    void postAlways(auto &&f) {
      io_->post(std::forward<decltype(f)>(f));
    }

    friend void post(PoolHandlerReady &self, auto &&f) {
      auto &pending_ = self.pending_;
      SAFE_UNIQUE(pending_) {
        if (pending_) {
          pending_->emplace_back(std::forward<decltype(f)>(f));
        } else if (not self.stopped_.test()) {
          self.postAlways(std::forward<decltype(f)>(f));
        }
      };
    }

    friend bool runningInThisThread(const PoolHandlerReady &self) {
      return kagome::runningInThisThread(self.io_);
    }

    // https://github.com/qdrvm/kagome/pull/2024#discussion_r1553225410
    void execute(auto &&f) {
      post(*this, std::forward<decltype(f)>(f));
    }

    // https://github.com/qdrvm/kagome/pull/2024#discussion_r1553225410
    bool isInCurrentThread() const {
      return runningInThisThread(*this);
    }

   private:
    std::shared_ptr<boost::asio::io_context> io_;
    using Pending = std::deque<std::function<void()>>;
    SafeObject<std::optional<Pending>> pending_{Pending{}};
    std::atomic_flag stopped_ = ATOMIC_FLAG_INIT;
  };
}  // namespace kagome
