/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <atomic>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <memory>
#include <optional>
#include <thread>

#include "soralog/util.hpp"
#include "utils/non_copyable.hpp"

namespace kagome {

  class ThreadHandler final {
    enum struct State : uint32_t { kStopped = 0, kStarted };

   public:
    ThreadHandler(ThreadHandler &&) = delete;
    ThreadHandler(const ThreadHandler &) = delete;

    ThreadHandler &operator=(ThreadHandler &&) = delete;
    ThreadHandler &operator=(const ThreadHandler &) = delete;

    explicit ThreadHandler(std::shared_ptr<boost::asio::io_context> io_context)
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
      BOOST_ASSERT(ioc_);
      if (State::kStarted == execution_state_.load(std::memory_order_acquire)) {
        ioc_->post(std::forward<F>(func));
      }
    }

    bool isInCurrentThread() const {
      BOOST_ASSERT(ioc_);
      return ioc_->get_executor().running_in_this_thread();
    }

    std::shared_ptr<boost::asio::io_context> io_context() const {
      return ioc_;
    }

   private:
    std::atomic<State> execution_state_;
    std::shared_ptr<boost::asio::io_context> ioc_;
  };

  /**
   * Creates `io_context` and runs it on `thread_count` threads.
   */
  class ThreadPool final {
    enum struct State : uint32_t { kStopped = 0, kStarted };

   public:
    ThreadPool(ThreadPool &&) = delete;
    ThreadPool(const ThreadPool &) = delete;

    ThreadPool &operator=(ThreadPool &&) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;

    [[deprecated("Please, use ctor for pool with named threads")]]  //
    explicit ThreadPool(size_t thread_count)
        : ioc_{std::make_shared<boost::asio::io_context>()},
          work_guard_{ioc_->get_executor()} {
      BOOST_ASSERT(ioc_);
      BOOST_ASSERT(thread_count > 0);

      static size_t pool_count = 0;

      threads_.reserve(thread_count);
      for (size_t i = 0; i < thread_count; ++i) {
        threads_.emplace_back(
            [io{ioc_}, thread_count, pool_n = ++pool_count, thread_n = i + 1] {
              if (thread_count > 1) {
                soralog::util::setThreadName(
                    fmt::format("worker.{}.{}", pool_n, thread_n));
              } else {
                soralog::util::setThreadName(fmt::format("worker.{}", pool_n));
              }
              io->run();
            });
      }
    }

    ThreadPool(std::string_view pool_tag, size_t thread_count)
        : ioc_{std::make_shared<boost::asio::io_context>()},
          work_guard_{ioc_->get_executor()} {
      BOOST_ASSERT(ioc_);
      BOOST_ASSERT(thread_count > 0);

      threads_.reserve(thread_count);
      for (size_t i = 0; i < thread_count; ++i) {
        threads_.emplace_back([io{ioc_},
                               pool_tag = std::string(pool_tag),
                               thread_count,
                               n = i + 1] {
          if (thread_count > 1) {
            soralog::util::setThreadName(fmt::format("{}.{}", pool_tag, n));
          } else {
            soralog::util::setThreadName(pool_tag);
          }
          io->run();
        });
      }
    }

    ~ThreadPool() {
      ioc_->stop();
      for (auto &thread : threads_) {
        thread.join();
      }
    }

    const std::shared_ptr<boost::asio::io_context> &io_context() const {
      return ioc_;
    }

    std::shared_ptr<ThreadHandler> handler() {
      BOOST_ASSERT(ioc_);
      return std::make_shared<ThreadHandler>(ioc_);
    }

   private:
    std::shared_ptr<boost::asio::io_context> ioc_;
    std::optional<boost::asio::executor_work_guard<
        boost::asio::io_context::executor_type>>
        work_guard_;
    std::vector<std::thread> threads_;
  };
}  // namespace kagome

#define REINVOKE(ctx, func, ...)                                             \
  do {                                                                       \
    if (not(ctx).isInCurrentThread()) {                                      \
      return (ctx).execute([weak = weak_from_this(),                         \
                            args = std::make_tuple(__VA_ARGS__)]() mutable { \
        if (auto self = weak.lock()) {                                       \
          std::apply(                                                        \
              [&](auto &&...args) mutable {                                  \
                self->func(std::forward<decltype(args)>(args)...);           \
              },                                                             \
              std::move(args));                                              \
        }                                                                    \
      });                                                                    \
    }                                                                        \
  } while (false)
