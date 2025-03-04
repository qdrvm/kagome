/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <fmt/format.h>

#include <memory>

template <typename T>
struct fmt::formatter<std::reference_wrapper<T>> : fmt::formatter<T> {
  template <typename FormatContext>
  auto format(const std::reference_wrapper<T> &ref, FormatContext &ctx) {
    if (ref.has_value()) {
      return fmt::formatter<T>::format(ref.value, ctx);
    }
    static constexpr string_view message("<dangling>");
    return std::ranges::copy(message, ctx.out());
  }
};

template <typename T>
struct fmt::formatter<std::shared_ptr<T>> : fmt::formatter<T> {
  template <typename FormatContext>
  auto format(const std::shared_ptr<T> &sptr, FormatContext &ctx) {
    if (sptr) {
      return fmt::formatter<T>::format(*sptr, ctx);
    }
    static constexpr string_view message("<nullptr>");
    return std::ranges::copy(message, ctx.out());
  }
};

template <typename T>
struct fmt::formatter<std::unique_ptr<T>> : fmt::formatter<T> {
  template <typename FormatContext>
  auto format(const std::unique_ptr<T> &uptr, FormatContext &ctx) {
    if (uptr) {
      return fmt::formatter<T>::format(*uptr, ctx);
    }
    static constexpr string_view message("<nullptr>");
    return std::ranges::copy(message, ctx.out());
  }
};

template <typename T>
struct fmt::formatter<std::weak_ptr<T>> : fmt::formatter<T> {
  template <typename FormatContext>
  auto format(const std::weak_ptr<T> &wptr, FormatContext &ctx) {
    auto sptr = wptr.lock();
    if (sptr) {
      return fmt::formatter<T>::format(*sptr, ctx);
    }
    static constexpr string_view message("<expired>");
    return std::ranges::copy(message, ctx.out());
  }
};
