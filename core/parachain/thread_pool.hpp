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

namespace kagome::thread {

  template <typename T>
  struct ThreadQueueContext {
    template <typename D>
    [[maybe_unused]] ThreadQueueContext(D &&);
    template <typename F>
    [[maybe_unused]] void make_call(F &&func);
  };

  template <typename T>
  inline auto createThreadQueueContext(T &&t) {
    return ThreadQueueContext<std::decay_t<T>>{std::forward<T>(t)};
  }

  template <>
  struct ThreadQueueContext<std::weak_ptr<boost::asio::io_context>> {
    using Type = std::weak_ptr<boost::asio::io_context>;
    Type t;

    template <typename D>
    ThreadQueueContext(D &&arg) : t{std::forward<D>(arg)} {}

    template <typename F>
    void make_call(F &&func) {
      if (auto call_context = t.lock()) {
        boost::asio::post(*call_context, std::forward<F>(func));
      }
    }
  };

  template <>
  struct ThreadQueueContext<std::shared_ptr<boost::asio::io_context>> {
    using Type = ThreadQueueContext<std::weak_ptr<boost::asio::io_context>>;
    Type t;

    template <typename D>
    ThreadQueueContext(D &&arg) : t{std::forward<D>(arg)} {}

    template <typename F>
    void make_call(F &&func) {
      return t.template make_call(std::forward<F>(func));
    }
  };

  struct ThreadPool final : std::enable_shared_from_this<ThreadPool> {
    using WorkersContext = boost::asio::io_context;
    using WorkGuard = boost::asio::executor_work_guard<
        boost::asio::io_context::executor_type>;

    ThreadPool() = delete;
    ThreadPool(const ThreadPool &) = delete;
    ThreadPool(ThreadPool &&) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;
    ThreadPool &operator=(ThreadPool &&) = delete;

    ThreadPool(std::shared_ptr<application::AppStateManager> asmgr,
               size_t thread_count = 5ull)
        : thread_count_{thread_count} {
      BOOST_ASSERT(thread_count_ > 0);
      asmgr->takeControl(*this);
    }

    ThreadPool(size_t thread_count = 5ull) : thread_count_{thread_count} {
      BOOST_ASSERT(thread_count_ > 0);
    }

    ~ThreadPool() {
      /// check that all workers are stopped.
      BOOST_ASSERT(workers_.empty());
    }

    bool prepare() {
      context_ = std::make_shared<WorkersContext>();
      work_guard_ = std::make_shared<WorkGuard>(context_->get_executor());
      workers_.reserve(thread_count_);
      return true;
    }

    bool start() {
      BOOST_ASSERT(context_);
      BOOST_ASSERT(work_guard_);
      for (size_t ix = 0; ix < thread_count_; ++ix) {
        workers_.emplace_back(
            [wptr{this->weak_from_this()}, context{context_}]() {
              if (auto self = wptr.lock()) {
                self->logger_->debug("Started thread worker with id: {}",
                                     std::this_thread::get_id());
              }
              context->run();
            });
      }
      return true;
    }

    void stop() {
      work_guard_.reset();
      if (context_) {
        context_->stop();
      }
      for (auto &worker : workers_) {
        if (worker.joinable()) {
          worker.join();
        }
      }
      workers_.clear();
    }

    template <typename T, typename K, typename... Args>
    void execute(std::pair<T, K> &&t, Args &&...args) {
      contextCall(
          std::move(t.first),
          [func{std::move(t.second)},
           forwarding_func{bindArgs(std::forward<Args>(args)...)}]() mutable {
            forwarding_func(func());
          });
    }

    template <typename T, typename K>
    void execute(std::pair<T, K> &&t) {
      contextCall(std::move(t.first), std::move(t.second));
    }

    template <typename F, typename... Args>
    void execute(F &&func, Args &&...args) {
      contextCall(
          [func{std::forward<F>(func)},
           forwarding_func{bindArgs(std::forward<Args>(args)...)}]() mutable {
            forwarding_func(func());
          });
    }

    template <typename F>
    void execute(F &&func) {
      contextCall(std::forward<F>(func));
    }

   private:
    friend struct ThreadQueueContext<ThreadPool>;

    template <typename T, typename F>
    void contextCall(ThreadQueueContext<T> &&t, F &&f) {
      t.make_call(std::forward<F>(f));
    }

    template <typename F>
    void contextCall(F &&f) {
      ThreadQueueContext<std::weak_ptr<WorkersContext>>(context_)
          .template make_call(std::forward<F>(f));
    }

    template <typename R, typename T, typename K>
    void executeI(outcome::result<R> &&r, std::pair<T, K> &&t) {
      if (r.has_value()) {
        contextCall(
            std::move(t.first),
            [r{std::move(r.value())}, func{std::move(t.second)}]() mutable {
              func(std::move(r));
            });
      }
    }

    template <typename R, typename F>
    void executeI(outcome::result<R> &&r, F &&func) {
      if (r.has_value()) {
        contextCall(
            [r{std::move(r.value())}, func{std::forward<F>(func)}]() mutable {
              std::forward<F>(func)(std::move(r));
            });
      }
    }

    template <typename R, typename T, typename K, typename... Args>
    void executeI(outcome::result<R> &&r, std::pair<T, K> &&t, Args &&...args) {
      if (r.has_value()) {
        contextCall(
            std::move(t.first),
            [func{std::move(t.second)},
             r{std::move(r.value())},
             forwarding_func{bindArgs(std::forward<Args>(args)...)}]() mutable {
              forwarding_func(func(std::move(r)));
            });
      }
    }

    template <typename R, typename F, typename... Args>
    void executeI(outcome::result<R> &&r, F &&func, Args &&...args) {
      if (r.has_value()) {
        contextCall(
            [func{std::forward<F>(func)},
             r{std::move(r.value())},
             forwarding_func{bindArgs(std::forward<Args>(args)...)}]() mutable {
              forwarding_func(std::forward<F>(func)(std::move(r)));
            });
      }
    }

    template <typename... Args>
    auto bindArgs(Args &&...args) {
      return std::bind(
          [wptr{weak_from_this()}](auto &&...args) mutable {
            if (auto self = wptr.lock()) self->executeI(std::move(args)...);
          },
          std::placeholders::_1,
          std::move(args)...);
    }

    const size_t thread_count_;
    std::shared_ptr<WorkersContext> context_;
    std::shared_ptr<WorkGuard> work_guard_;
    std::vector<std::thread> workers_;
    log::Logger logger_ = log::createLogger("ThreadPool", "thread");
  };

  template <>
  struct ThreadQueueContext<ThreadPool> {
    using Type = ThreadQueueContext<std::weak_ptr<boost::asio::io_context>>;
    Type t;

    ThreadQueueContext(const ThreadPool &arg) : t{arg.context_} {}

    template <typename F>
    void make_call(F &&func) {
      return t.template make_call(std::forward<F>(func));
    }
  };

}  // namespace kagome::thread

#endif  // KAGOME_THREAD_POOL_HPP
