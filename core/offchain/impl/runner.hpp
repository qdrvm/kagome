/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <deque>
#include <mutex>

#include <libp2p/common/final_action.hpp>

#include "utils/thread_pool.hpp"

namespace kagome::offchain {
  constexpr size_t kMaxThreads = 3;
  constexpr size_t kMaxTasks = 1000;

  /**
   * Enqueue at most `max_tasks_` to run on number of `threads_`.
   * Old tasks do not run and are removed when queue is full.
   */
  class Runner : public std::enable_shared_from_this<Runner> {
   public:
    using Task = std::function<void()>;

    Runner(size_t threads = kMaxThreads, size_t max_tasks = kMaxTasks)
        : threads_{threads},
          free_threads_{threads},
          max_tasks_{max_tasks},
          thread_pool_{"ocw", threads_} {}

    struct Inject {
      explicit Inject() = default;
    };
    Runner(Inject, ...) : Runner(kMaxThreads, kMaxTasks) {}

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
      thread_pool_.io_context()->post(
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
    const size_t threads_;
    size_t free_threads_;
    const size_t max_tasks_;
    std::deque<Task> tasks_;
    ThreadPool thread_pool_;
  };
}  // namespace kagome::offchain
