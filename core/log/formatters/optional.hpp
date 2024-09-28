/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <fmt/format.h>

#include <optional>
#include <ranges>
#include <string_view>

#include <boost/optional.hpp>

template <template <typename> class Optional, typename T>
  requires std::is_same_v<Optional<T>, std::optional<T>>
        or std::is_same_v<Optional<T>, boost::optional<T>>
struct fmt::formatter<Optional<T>> {
  constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin()) {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const Optional<T> &opt, FormatContext &ctx) const
      -> decltype(ctx.out()) {
    // ctx.out() is an output iterator to write to.

    if (opt.has_value()) {
      return fmt::format_to(ctx.out(), "{}", opt.value());
    }
    static constexpr std::string_view message("<none>");
    return std::copy(std::begin(message), std::end(message), ctx.out());
  }
};
