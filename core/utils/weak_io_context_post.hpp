/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/asio/io_context.hpp>

#include "utils/weak_io_context.hpp"

namespace kagome {
  void post(const WeakIoContext &weak, auto f) {
    if (auto io = weak.lock()) {
      io->post(std::move(f));
    }
  }

  inline bool runningInThisThread(const WeakIoContext &weak) {
    auto io = weak.lock();
    return io and io->get_executor().running_in_this_thread();
  }

  auto wrap(const WeakIoContext &weak, auto f) {
    return [weak, f{std::move(f)}](auto &&...a) mutable {
      post(weak,
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
                  [weak = weak_from_this(),                                    \
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
