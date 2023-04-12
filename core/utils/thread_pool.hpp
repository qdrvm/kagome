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
    ThreadHandler(ThreadHandler const &) = delete;

    ThreadHandler &operator=(ThreadHandler &&) = delete;
    ThreadHandler &operator=(ThreadHandler const &) = delete;

    ThreadHandler()
        : execution_state_{State::kStopped},
          ioc_{std::make_shared<boost::asio::io_context>()},
          work_guard_{ioc_->get_executor()} {}

    explicit ThreadHandler(std::shared_ptr<boost::asio::io_context> io_context)
        : execution_state_{State::kStopped},
          ioc_{std::move(io_context)},
          work_guard_{} {}

    ~ThreadHandler() {
      if (work_guard_) {
        ioc_->stop();
      }
    }

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
    std::optional<boost::asio::executor_work_guard<
        boost::asio::io_context::executor_type>>
        work_guard_;
  };

  /**
   * Creates `io_context` and runs it on `thread_count` threads.
   */
  class ThreadPool final {
    enum struct State : uint32_t { kStopped = 0, kStarted };

   public:
    ThreadPool(ThreadPool &&) = delete;
    ThreadPool(ThreadPool const &) = delete;

    ThreadPool &operator=(ThreadPool &&) = delete;
    ThreadPool &operator=(ThreadPool const &) = delete;

    explicit ThreadPool(size_t thread_count)
        : handler_{std::make_optional<ThreadHandler>()} {
      BOOST_ASSERT(handler_);
      BOOST_ASSERT(thread_count > 0);
      threads_.reserve(thread_count);

      for (size_t i = 0; i < thread_count; ++i) {
        threads_.emplace_back([io{handler_->io_context()}] { io->run(); });
      }
    }

    ~ThreadPool() {
      BOOST_ASSERT(handler_);
      handler_ = std::nullopt;

      BOOST_ASSERT(!threads_.empty());
      for (auto &thread : threads_) {
        thread.join();
      }
    }

    template <typename F>
    void execute(F &&func) {
      BOOST_ASSERT(handler_);
      handler_->execute(std::forward<F>(func));
    }

    template <typename F, typename... Args>
    auto reinvoke(F &&func, Args &&...args)
        -> std::optional<std::tuple<Args...>> {
      BOOST_ASSERT(handler_);
      return handler_->reinvoke(std::forward<F>(func),
                                std::forward<Args>(args)...);
    }

    bool isInCurrentThread() const {
      BOOST_ASSERT(handler_);
      return handler_->isInCurrentThread();
    }

    ThreadHandler &handler() {
      BOOST_ASSERT(handler_);
      return *handler_;
    }

   private:
    std::optional<ThreadHandler> handler_;
    std::vector<std::thread> threads_;
  };
}  // namespace kagome

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
