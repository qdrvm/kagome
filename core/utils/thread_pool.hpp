/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_UTILS_THREAD_POOL_HPP
#define KAGOME_UTILS_THREAD_POOL_HPP

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <memory>
#include <thread>

#include "utils/non_copyable.hpp"

namespace kagome {
  // TODO(turuslan): use `ThreadPool` in `RpcThreadPool` and `RpcContext`

  /**
   * Creates `io_context` and runs it on `thread_count` threads.
   */
  class ThreadPool final : NonCopyable, NonMovable {
    ThreadPool(size_t thread_count)
        : io_context_{std::make_shared<boost::asio::io_context>()},
          work_guard_{io_context_->get_executor()} {
      BOOST_ASSERT(thread_count > 0);
      threads_.reserve(thread_count);
      for (size_t i = 0; i < thread_count; ++i) {
        threads_.emplace_back([io{io_context_}] { io->run(); });
      }
    }

    explicit ThreadPool(size_t thread_count)
        : ThreadPool{thread_count, [](size_t) {}} {}

    ~ThreadPool() {
      io_context_->stop();
      for (auto &thread : threads_) {
        thread.join();
      }
    }

    const std::shared_ptr<boost::asio::io_context> &io_context() const {
      return io_context_;
    }

   private:
    std::shared_ptr<boost::asio::io_context> io_context_;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type>
        work_guard_;
    std::vector<std::thread> threads_;
  };
}  // namespace kagome

#endif  // KAGOME_UTILS_THREAD_POOL_HPP
