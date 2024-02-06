/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/asio/io_context.hpp>
#include <soralog/util.hpp>

#include "log/logger.hpp"
#include "utils/thread_handler.hpp"
#include "utils/watchdog.hpp"

namespace kagome {

  /**
   * Creates `io_context` and runs it on `thread_count` threads.
   */
  class ThreadPool {
    enum struct State : uint32_t { kStopped = 0, kStarted };

   public:
    ThreadPool(ThreadPool &&) = delete;
    ThreadPool(const ThreadPool &) = delete;

    ThreadPool &operator=(ThreadPool &&) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;

    // Next nested struct and deleted ctor added to avoid unintended injections
    struct Inject {
      explicit Inject() = default;
    };
    explicit ThreadPool(Inject, ...);

    ThreadPool(std::shared_ptr<Watchdog> watchdog,
               std::string_view pool_tag,
               size_t thread_count,
               std::optional<std::shared_ptr<boost::asio::io_context>> ioc)
        : log_(log::createLogger(fmt::format("ThreadPool:{}", pool_tag),
                                 "threads")),
          ioc_{ioc.has_value() ? std::move(ioc.value())
                               : std::make_shared<boost::asio::io_context>()},
          work_guard_{ioc_->get_executor()} {
      BOOST_ASSERT(ioc_);
      BOOST_ASSERT(thread_count > 0);

      SL_TRACE(log_, "Pool created");
      threads_.reserve(thread_count);
      for (size_t i = 0; i < thread_count; ++i) {
        std::string label(thread_count > 1
                              ? fmt::format("{}.{}", pool_tag, i + 1)
                              : pool_tag);
        threads_.emplace_back(
            [log(log_), io{ioc_}, watchdog, label{std::move(label)}] {
              soralog::util::setThreadName(label);
              SL_TRACE(log, "Thread '{}' started", label);
              watchdog->run(io);
              SL_TRACE(log, "Thread '{}' stopped", label);
            });
      }
    }

    virtual ~ThreadPool() {
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
