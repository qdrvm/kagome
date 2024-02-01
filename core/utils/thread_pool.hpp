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

#include "log/logger.hpp"
#include "utils/watchdog.hpp"
#include "utils/weak_io_context_post.hpp"

namespace kagome {

  class ThreadHandler final {
    enum struct State : uint32_t { kStopped = 0, kStarted };

   public:
    ThreadHandler(ThreadHandler &&) = delete;
    ThreadHandler(const ThreadHandler &) = delete;

    ThreadHandler &operator=(ThreadHandler &&) = delete;
    ThreadHandler &operator=(const ThreadHandler &) = delete;

    explicit ThreadHandler(WeakIoContext io_context)
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

   private:
    std::atomic<State> execution_state_;
    WeakIoContext ioc_;
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

    ThreadPool(std::shared_ptr<Watchdog> watchdog,
               std::string_view pool_tag,
               size_t thread_count,
               std::optional<std::shared_ptr<boost::asio::io_context>> ioc =
                   std::nullopt)
        : log_(log::createLogger(fmt::format("ThreadPool:{}", pool_tag),
                                 "threads",
                                 log::Level::TRACE)),  // FIXME
          ioc_{ioc.has_value() ? std::move(ioc.value())
                               : std::make_shared<boost::asio::io_context>()},
          work_guard_{ioc_->get_executor()} {
      BOOST_ASSERT(ioc_);
      BOOST_ASSERT(thread_count > 0);

      SL_TRACE(log_, "Pool created");
      threads_.reserve(thread_count);
      for (size_t i = 0; i < thread_count; ++i) {
        threads_.emplace_back([log(log_),
                               io{ioc_},
                               watchdog,
                               pool_tag = std::string(pool_tag),
                               thread_count,
                               n = i + 1] {
          auto label =
              thread_count > 1 ? fmt::format("{}.{}", pool_tag, n) : pool_tag;
          soralog::util::setThreadName(label);
          SL_TRACE(log, "Thread {} started", label);
          watchdog->run(io);
          SL_TRACE(log, "Thread {} stopped", label);
        });
      }
    }

    ~ThreadPool() {
      SL_TRACE(log_, "DTor called");
      ioc_->stop();
      SL_TRACE(log_, "Ctx stopped");
      for (auto &thread : threads_) {
        SL_TRACE(log_, "Joining thread...");
        thread.join();
      }
      SL_TRACE(log_, "Pool destroyed");
    }

    const std::shared_ptr<boost::asio::io_context> &io_context() const {
      return ioc_;
    }

    std::shared_ptr<ThreadHandler> handler() {
      BOOST_ASSERT(ioc_);
      return std::make_shared<ThreadHandler>(ioc_);
    }

   private:
    log::Logger log_;
    std::shared_ptr<boost::asio::io_context> ioc_;
    std::optional<boost::asio::executor_work_guard<
        boost::asio::io_context::executor_type>>
        work_guard_;
    std::vector<std::thread> threads_;
  };
}  // namespace kagome
