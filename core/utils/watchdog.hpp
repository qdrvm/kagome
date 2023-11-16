/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <atomic>
#include <list>
#include <memory>
#include <mutex>
#include <sstream>
#include <thread>
#include <unordered_map>

#include <fmt/format.h>
#include <boost/asio/io_context.hpp>

#ifdef __APPLE__

#include <mach/mach.h>
#include <mach/thread_act.h>
#include <mach/thread_info.h>

inline uint64_t getPlatformThreadId() {
  thread_identifier_info_data_t info;
  mach_msg_type_number_t size = THREAD_IDENTIFIER_INFO_COUNT;
  auto r = thread_info(
      mach_thread_self(), THREAD_IDENTIFIER_INFO, (thread_info_t)&info, &size);
  if (r != KERN_SUCCESS) {
    throw std::logic_error{"thread_info"};
  }
  return info.thread_id;
}

#else

#include <sys/syscall.h>
#include <unistd.h>

inline uint64_t getPlatformThreadId() {
  return syscall(SYS_gettid);
}

#endif

namespace kagome {

  constexpr auto kWatchdogDefaultTimeout = std::chrono::minutes{15};

  class Watchdog {
   public:
    using Count = uint32_t;
    using Atomic = std::atomic<Count>;
    using Clock = std::chrono::steady_clock;
    using Timeout = std::chrono::seconds;

    struct Ping {
      std::shared_ptr<Atomic> count_;

      void operator()() {
        ++*count_;
      }
    };

    void checkLoop(Timeout timeout) {
      // or `io_context` with timer
      while (not stopped_) {
        std::this_thread::sleep_for(std::chrono::seconds{1});
        check(timeout);
      }
    }

    // periodic check
    void check(Timeout timeout) {
      std::unique_lock lock{mutex_};
      auto now = Clock::now();
      for (auto it = threads_.begin(); it != threads_.end();) {
        if (it->second.count.unique()) {
          it = threads_.erase(it);
        } else {
          auto count = it->second.count->load();
          if (it->second.last_count != count) {
            it->second.last_count = count;
            it->second.last_time = now;
          } else {
            auto lag = now - it->second.last_time;
            if (lag > timeout) {
              std::stringstream s;
              s << it->first;
              fmt::print(
                  "ALERT Watchdog: thread id={}, platform_id={}, name={} "
                  "timeout\n",
                  s.str(),
                  it->second.platform_id,
                  it->second.name);
              std::abort();
            }
          }
          ++it;
        }
      }
    }

    [[nodiscard]] Ping add() {
      std::unique_lock lock{mutex_};
      auto &thread = threads_[std::this_thread::get_id()];
      if (not thread.count) {
        thread = {Clock::now(),
                  0,
                  std::make_shared<Atomic>(),
                  getPlatformThreadId(),
                  soralog::util::getThreadName()};
      }
      return Ping{thread.count};
    }

    void run(std::shared_ptr<boost::asio::io_context> io) {
      auto ping = add();
      while (not stopped_ and not io.unique()) {
#if 0
        // this is the desired implementation
        // cannot be used rn due to the method visibility settings
        // bug(boost): wait_one is private
        boost::system::error_code ec;
        io->impl_.wait_one(TODO, ec);
        ping();
        io->poll_one();
#else
        // `run_one_for` run time is sum of `wait_one(time)` and `poll_one`
        // may cause false-positive timeout
        io->run_one_for(std::chrono::seconds{1});
#endif
        ping();
        io->restart();
      }
    }

    void stop() {
      stopped_ = true;
    }

   private:
    struct Thread {
      Clock::time_point last_time;
      Count last_count = 0;
      std::shared_ptr<Atomic> count;
      uint64_t platform_id;
      std::string name;
    };

    std::mutex mutex_;
    std::unordered_map<std::thread::id, Thread> threads_;
    std::atomic_bool stopped_ = false;
  };
}  // namespace kagome
