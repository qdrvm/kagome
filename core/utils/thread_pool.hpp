/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_UTILS_THREAD_POOL_HPP
#define KAGOME_UTILS_THREAD_POOL_HPP

#include <atomic>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <memory>
#include <thread>

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
      execution_state_.store(State::kStarted, std::memory_order_release);
    }

    void stop() {
      execution_state_.store(State::kStopped, std::memory_order_release);
    }

    template <typename F>
    void execute(F &&func) {
      BOOST_ASSERT(ioc_);
      if (State::kStarted == execution_state_.load(std::memory_order_acquire)) {
        ioc_->post(std::forward<F>(func));
      }
    }

    template <typename F, typename... Args>
    auto reinvoke(F &&func, Args &&...args)
        -> std::optional<std::tuple<Args...>> {
      if (!isInCurrentThread()) {
        execute([func(std::forward<F>(func)),
                 tup{std::tuple<std::decay_t<Args>...>{
                     std::forward<Args>(args)...}}]() mutable {
          std::apply([&](auto &&...a) mutable { func(std::move(a)...); },
                     std::move(tup));
        });
        return std::nullopt;
      }
      return {std::make_tuple(std::forward<Args>(args)...)};
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

    explicit ThreadPool(size_t thread_count)
        : ioc_{std::make_shared<boost::asio::io_context>()},
          work_guard_{ioc_->get_executor()} {
      BOOST_ASSERT(ioc_);
      BOOST_ASSERT(thread_count > 0);

      threads_.reserve(thread_count);
      for (size_t i = 0; i < thread_count; ++i) {
        threads_.emplace_back([io{ioc_}] { io->run(); });
      }
    }

    ~ThreadPool() {
      ioc_->stop();
      for (auto &thread : threads_) {
        thread.join();
      }
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

#define REINVOKE_6(ctx,                                                       \
                   func,                                                      \
                   in_1,                                                      \
                   in_2,                                                      \
                   in_3,                                                      \
                   in_4,                                                      \
                   in_5,                                                      \
                   in_6,                                                      \
                   out_1,                                                     \
                   out_2,                                                     \
                   out_3,                                                     \
                   out_4,                                                     \
                   out_5,                                                     \
                   out_6)                                                     \
  auto res##func = (ctx).reinvoke(                                            \
      [wself(weak_from_this())](                                              \
          auto &&x1, auto &&x2, auto &&x3, auto &&x4, auto &&x5, auto &&x6) { \
        if (auto self = wself.lock()) {                                       \
          self->func(std::move(x1),                                           \
                     std::move(x2),                                           \
                     std::move(x3),                                           \
                     std::move(x4),                                           \
                     std::move(x5),                                           \
                     std::move(x6));                                          \
        }                                                                     \
      },                                                                      \
      std::move(in_1),                                                        \
      std::move(in_2),                                                        \
      std::move(in_3),                                                        \
      std::move(in_4),                                                        \
      std::move(in_5),                                                        \
      std::move(in_6));                                                       \
  if (!(res##func)) return;                                                   \
  auto &&[func##x1, func##x2, func##x3, func##x4, func##x5, func##x6] =       \
      std::move(*(res##func));                                                \
  auto && (out_1) = func##x1;                                                 \
  auto && (out_2) = func##x2;                                                 \
  auto && (out_3) = func##x3;                                                 \
  auto && (out_4) = func##x4;                                                 \
  auto && (out_5) = func##x5;                                                 \
  auto && (out_6) = func##x6;

#define REINVOKE_4(                                                           \
    ctx, func, in_1, in_2, in_3, in_4, out_1, out_2, out_3, out_4)            \
  auto res##func = (ctx).reinvoke(                                            \
      [wself(weak_from_this())](auto &&x1, auto &&x2, auto &&x3, auto &&x4) { \
        if (auto self = wself.lock()) {                                       \
          self->func(                                                         \
              std::move(x1), std::move(x2), std::move(x3), std::move(x4));    \
        }                                                                     \
      },                                                                      \
      std::move(in_1),                                                        \
      std::move(in_2),                                                        \
      std::move(in_3),                                                        \
      std::move(in_4));                                                       \
  if (!(res##func)) return;                                                   \
  auto &&[func##x1, func##x2, func##x3, func##x4] = std::move(*(res##func));  \
  auto && (out_1) = func##x1;                                                 \
  auto && (out_2) = func##x2;                                                 \
  auto && (out_3) = func##x3;                                                 \
  auto && (out_4) = func##x4;

#define REINVOKE_3(ctx, func, in_1, in_2, in_3, out_1, out_2, out_3) \
  auto res##func = (ctx).reinvoke(                                   \
      [wself(weak_from_this())](auto &&x1, auto &&x2, auto &&x3) {   \
        if (auto self = wself.lock()) {                              \
          self->func(std::move(x1), std::move(x2), std::move(x3));   \
        }                                                            \
      },                                                             \
      std::move(in_1),                                               \
      std::move(in_2),                                               \
      std::move(in_3));                                              \
  if (!(res##func)) return;                                          \
  auto &&[func##x1, func##x2, func##x3] = std::move(*(res##func));   \
  auto && (out_1) = func##x1;                                        \
  auto && (out_2) = func##x2;                                        \
  auto && (out_3) = func##x3;

#define REINVOKE_2(ctx, func, in_1, in_2, out_1, out_2)  \
  auto res##func = (ctx).reinvoke(                       \
      [wself(weak_from_this())](auto &&x1, auto &&x2) {  \
        if (auto self = wself.lock()) {                  \
          self->func(std::move(x1), std::move(x2));      \
        }                                                \
      },                                                 \
      std::move(in_1),                                   \
      std::move(in_2));                                  \
  if (!(res##func)) return;                              \
  auto &&[func##x1, func##x2] = std::move(*(res##func)); \
  auto && (out_1) = func##x1;                            \
  auto && (out_2) = func##x2;

#define REINVOKE_1(ctx, func, in, out)      \
  auto res##func = (ctx).reinvoke(          \
      [wself(weak_from_this())](auto &&x) { \
        if (auto self = wself.lock()) {     \
          self->func(std::move(x));         \
        }                                   \
      },                                    \
      std::move(in));                       \
  if (!(res##func)) return;                 \
  auto && (out) = std::get<0>(std::move(*(res##func)));

#endif  // KAGOME_UTILS_THREAD_POOL_HPP
