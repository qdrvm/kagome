/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <atomic>
#include <memory>
#include <optional>
#include <thread>

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <soralog/util.hpp>

#include "utils/watchdog.hpp"
#include "utils/weak_io_context_post.hpp"

namespace kagome {

  class ThreadHandler final {
    enum struct State : uint32_t { kStopped = 0, kStarted };

   public:
    struct Locked {
      virtual ~Locked() = default;
      virtual const std::shared_ptr<boost::asio::io_context> &io_context()
          const = 0;
    };

    ThreadHandler(ThreadHandler &&) = delete;
    ThreadHandler(const ThreadHandler &) = delete;

    ThreadHandler &operator=(ThreadHandler &&) = delete;
    ThreadHandler &operator=(const ThreadHandler &) = delete;

    explicit ThreadHandler(WeakIoContext io_context)
        : execution_state_{State::kStarted}, ioc_{std::move(io_context)} {}
    ThreadHandler(std::shared_ptr<Locked> locked)
        : execution_state_{State::kStarted},
          ioc_{locked->io_context()},
          locked_{locked} {}

    ~ThreadHandler() = default;

    void start() {
      execution_state_.store(State::kStarted);
    }

    void stop() {
      execution_state_.store(State::kStopped);
    }

    template <typename F>
    void execute(F &&func) {
      if (State::kStarted == execution_state_.load(std::memory_order_acquire)) {
        post(ioc_, std::forward<F>(func));
      }
    }

    friend void post(ThreadHandler &self, auto f) {
      return self.execute(std::move(f));
    }

    bool isInCurrentThread() const {
      return runningInThisThread(ioc_);
    }

    friend bool runningInThisThread(const ThreadHandler &self) {
      return self.isInCurrentThread();
    }

    std::shared_ptr<boost::asio::io_context> io_context() {
      return ioc_.lock();
    }

   private:
    std::atomic<State> execution_state_;
    WeakIoContext ioc_;
    std::shared_ptr<Locked> locked_;
  };

  /**
   * Creates `io_context` and runs it on `thread_count` threads.
   */
  class ThreadPool final : public std::enable_shared_from_this<ThreadPool>,
                           public ThreadHandler::Locked {
    enum struct State : uint32_t { kStopped = 0, kStarted };

    ThreadPool(std::shared_ptr<Watchdog> watchdog,
               std::string_view pool_tag,
               size_t thread_count)
        : ioc_{std::make_shared<boost::asio::io_context>()},
          work_guard_{ioc_->get_executor()} {
      BOOST_ASSERT(ioc_);
      BOOST_ASSERT(thread_count > 0);

      threads_.reserve(thread_count);
      for (size_t i = 0; i < thread_count; ++i) {
        threads_.emplace_back([io{ioc_},
                               watchdog,
                               pool_tag = std::string(pool_tag),
                               thread_count,
                               n = i + 1] {
          if (thread_count > 1) {
            soralog::util::setThreadName(fmt::format("{}.{}", pool_tag, n));
          } else {
            soralog::util::setThreadName(pool_tag);
          }
          watchdog->run(io);
        });
      }
    }

   public:
    ThreadPool(ThreadPool &&) = delete;
    ThreadPool(const ThreadPool &) = delete;

    ThreadPool &operator=(ThreadPool &&) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;

    ~ThreadPool() {
      ioc_->stop();
      for (auto &thread : threads_) {
        thread.join();
      }
    }

    /// to prevent creation without `shared_ptr`
    template <typename... Args>
    static std::shared_ptr<ThreadPool> create(Args &&...args) {
      return std::shared_ptr<ThreadPool>(
          new ThreadPool(std::forward<Args>(args)...));
    }

    const std::shared_ptr<boost::asio::io_context> &io_context()
        const override {
      return ioc_;
    }

    std::shared_ptr<ThreadHandler> handler() {
      return std::make_shared<ThreadHandler>(shared_from_this());
    }

   private:
    std::shared_ptr<boost::asio::io_context> ioc_;
    std::optional<boost::asio::executor_work_guard<
        boost::asio::io_context::executor_type>>
        work_guard_;
    std::vector<std::thread> threads_;
  };
}  // namespace kagome
