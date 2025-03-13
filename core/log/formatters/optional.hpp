/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <fmt/format.h>

#include <optional>

#include <boost/optional.hpp>

template <template <typename> class Optional, typename T>
  requires requires(Optional<T> opt) {
    { opt.has_value() } -> std::convertible_to<bool>;
    { *opt } -> std::convertible_to<T>;
  } and fmt::is_formattable<T>::value
struct fmt::formatter<Optional<T>> : fmt::formatter<T> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext &ctx) {
    return fmt::formatter<T>::parse(ctx);
  }

  template <typename FormatContext>
  auto format(const Optional<T> &opt, FormatContext &ctx) const {
    if (opt.has_value()) {
      return fmt::formatter<T>::format(opt.value(), ctx);
    }
    return fmt::format_to(ctx.out(), "<none>");
  }
};
