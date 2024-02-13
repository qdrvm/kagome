/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/asio/io_context.hpp>

namespace kagome {

  inline bool runningInThisThread(
      std::shared_ptr<boost::asio::io_context> ioc) {
    return ioc->get_executor().running_in_this_thread();
  }

  class PoolHandler {
   public:
    PoolHandler(PoolHandler &&) = delete;
    PoolHandler(const PoolHandler &) = delete;

    PoolHandler &operator=(PoolHandler &&) = delete;
    PoolHandler &operator=(const PoolHandler &) = delete;

    // Next nested struct and deleted ctor added to avoid unintended injections
    struct Inject {
      explicit Inject() = default;
    };
    explicit PoolHandler(Inject, ...);

    explicit PoolHandler(std::shared_ptr<boost::asio::io_context> io_context)
        : is_active_{false}, ioc_{std::move(io_context)} {}
    ~PoolHandler() = default;

    void start() {
      is_active_.store(true);
    }

    void stop() {
      is_active_.store(false);
    }

    template <typename F>
    void execute(F &&func) {
      if (is_active_.load(std::memory_order_acquire)) {
        ioc_->post(std::forward<F>(func));
      }
    }

    friend void post(PoolHandler &self, auto f) {
      return self.execute(std::move(f));
    }

    bool isInCurrentThread() const {
      return runningInThisThread(ioc_);
    }

    friend bool runningInThisThread(const PoolHandler &self) {
      return self.isInCurrentThread();
    }

   private:
    std::atomic_bool is_active_;
    std::shared_ptr<boost::asio::io_context> ioc_;
  };

  auto wrap(PoolHandler &handler, auto f) {
    return [&handler, f{std::move(f)}](auto &&...a) mutable {
      handler.execute(
          [f{std::move(f)}, ... a{std::forward<decltype(a)>(a)}]() mutable {
            f(std::forward<decltype(a)>(a)...);
          });
    };
  }

}  // namespace kagome

#define REINVOKE(ctx, func, ...)                                               \
  do {                                                                         \
    if (not runningInThisThread(ctx)) {                                        \
      return post(ctx,                                                         \
                  [weak{weak_from_this()},                                     \
                   args = std::make_tuple(__VA_ARGS__)]() mutable {            \
                    if (auto self = weak.lock()) {                             \
                      std::apply(                                              \
                          [&](auto &&...args) mutable {                        \
                            self->func(std::forward<decltype(args)>(args)...); \
                          },                                                   \
                          std::move(args));                                    \
                    }                                                          \
                  });                                                          \
    }                                                                          \
  } while (false)
