#pragma once

#include <fmt/format.h>
#include <atomic>
#include <boost/asio/io_context.hpp>
#include <list>
#include <memory>
#include <mutex>
#include <sstream>
#include <thread>
#include <unordered_map>

namespace kagome {
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
      while (true) {
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
              fmt::print("ALERT Watchdog: thread id={} timeout\n", s.str());
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
        thread = {Clock::now(), 0, std::make_shared<Atomic>()};
      }
      return Ping{thread.count};
    }

    void run(std::shared_ptr<boost::asio::io_context> io) {
      auto ping = add();
      while (not stopped_ and not io.unique()) {
#if 0
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
    };

    std::mutex mutex_;
    std::unordered_map<std::thread::id, Thread> threads_;
    bool stopped_ = false;
  };
}  // namespace kagome
