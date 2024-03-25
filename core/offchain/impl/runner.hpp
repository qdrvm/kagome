/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <deque>
#include <mutex>

#include <libp2p/common/final_action.hpp>

#include "application/app_state_manager.hpp"
#include "utils/thread_pool.hpp"

namespace kagome {
  class Watchdog;
}

namespace kagome::offchain {
  constexpr size_t kMaxThreads = 3;
  constexpr size_t kMaxTasks = 1000;

  class OcwThreadPool final : public ThreadPool {
   public:
    OcwThreadPool(std::shared_ptr<Watchdog> watchdog)
        : ThreadPool(std::move(watchdog), "ocw", kMaxThreads, std::nullopt) {}
  };

  /**
   * Enqueue at most `max_tasks_` to run on number of `threads_`.
   * Old tasks do not run and are removed when queue is full.
   */
  class Runner : public std::enable_shared_from_this<Runner> {
   public:
    using Task = std::function<void()>;

    Runner(application::AppStateManager &app_state_manager,
           OcwThreadPool &ocw_thread_pool)
        : free_threads_{kMaxThreads},
          max_tasks_{kMaxTasks},
          ocw_thread_handler_{ocw_thread_pool.handler(app_state_manager)} {}

    void run(Task &&task) {
      std::unique_lock lock{mutex_};
      if (tasks_.size() >= max_tasks_) {
        tasks_.pop_front();
      }
      if (free_threads_ == 0) {
        tasks_.emplace_back(std::move(task));
        return;
      }
      --free_threads_;
      lock.unlock();
      post(*ocw_thread_handler_,
           [weak{weak_from_this()}, task{std::move(task)}] {
             if (auto self = weak.lock()) {
               ::libp2p::common::FinalAction release([&] {
                 std::unique_lock lock{self->mutex_};
                 ++self->free_threads_;
               });
               task();
               self->drain();
             }
           });
    }

   private:
    void drain() {
      while (true) {
        std::unique_lock lock{mutex_};
        if (tasks_.empty()) {
          break;
        }
        auto task{std::move(tasks_.front())};
        tasks_.pop_front();
        lock.unlock();
        task();
      }
    }

    std::mutex mutex_;
    size_t free_threads_;
    const size_t max_tasks_;
    std::deque<Task> tasks_;
    std::shared_ptr<PoolHandler> ocw_thread_handler_;
  };
}  // namespace kagome::offchain
