/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/asio/io_context.hpp>

#include "injector/inject.hpp"

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

    DONT_INJECT(PoolHandler);

    explicit PoolHandler(std::shared_ptr<boost::asio::io_context> io_context)
        : is_active_{false}, ioc_{std::move(io_context)} {}
    ~PoolHandler() = default;

    void start() {
      started_ = true;
      is_active_.store(true);
    }

    void stop() {
      is_active_.store(false);
    }

    template <typename F>
    void execute(F &&func) {
      if (is_active_.load(std::memory_order_acquire)) {
        ioc_->post(std::forward<F>(func));
      } else if (not started_) {
        throw std::logic_error{"PoolHandler lost callback before start()"};
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
    std::atomic_bool started_ = false;
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

#define REINVOKE_ONCE(ctx, func, ...)                                         \
  do {                                                                        \
    if constexpr (kReinvoke) {                                                \
      return post(                                                            \
          ctx,                                                                \
          [weak{weak_from_this()},                                            \
           args = std::make_tuple(__VA_ARGS__)]() mutable {                   \
            if (auto self = weak.lock()) {                                    \
              std::apply(                                                     \
                  [&](auto &&...args) mutable {                               \
                    self->func<false>(std::forward<decltype(args)>(args)...); \
                  },                                                          \
                  std::move(args));                                           \
            }                                                                 \
          });                                                                 \
    }                                                                         \
  } while (false)
