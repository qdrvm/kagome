/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_THREAD_POOL_HPP
#define KAGOME_THREAD_POOL_HPP

#include <memory>
#include <queue>
#include <thread>
#include <type_traits>

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/signal_set.hpp>

#include "application/app_state_manager.hpp"
#include "parachain/tasks_sequence.hpp"

namespace kagome::thread {

  /**
   * Thread pool with sequenced task execution on different threads.
   */
  struct ThreadPool final : std::enable_shared_from_this<ThreadPool> {
    using WorkersContext = boost::asio::io_context;
    using WorkGuard = boost::asio::executor_work_guard<
        boost::asio::io_context::executor_type>;

    ThreadPool() = delete;
    ThreadPool(const ThreadPool &) = delete;
    ThreadPool(ThreadPool &&) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;
    ThreadPool &operator=(ThreadPool &&) = delete;

    ThreadPool(size_t thread_count = 5ull);
    ThreadPool(std::shared_ptr<application::AppStateManager> app_state_manager,
               size_t thread_count = 5ull);
    ~ThreadPool();

    /// AppStateManager Impl
    bool prepare();
    bool start();
    void stop();

    template <typename F>
    void pushTask(F &&f) {
      boost::asio::post(*context_, std::forward<F>(f));
    }

   private:
    friend struct ThreadQueueContext<ThreadPool>;

    const size_t thread_count_;
    std::shared_ptr<WorkersContext> context_;
    std::unique_ptr<WorkGuard> work_guard_;
    std::vector<std::thread> workers_;
    log::Logger logger_ = log::createLogger("ThreadPool", "thread");
  };

  /*
   * Wrapper for sequenced execution.
   * */
  template <>
  struct ThreadQueueContext<std::shared_ptr<ThreadPool>> {
    using Type = std::weak_ptr<ThreadPool>;
    Type t;

    ThreadQueueContext(const std::shared_ptr<ThreadPool> &arg) : t{arg} {}

    template <typename F>
    void operator()(F &&func) {
      if (auto tp = t.lock()) {
        tp->pushTask(std::forward<F>(func));
      }
    }
  };

}  // namespace kagome::thread

#endif  // KAGOME_THREAD_POOL_HPP
