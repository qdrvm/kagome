/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <fmt/core.h>

#include <optional>
#include <ranges>
#include <string_view>

#include <boost/optional.hpp>

template <typename T>
struct fmt::formatter<std::optional<T>> {
  constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin()) {
    // Parse the presentation format and store it in the formatter:
    auto it = ctx.begin(), end = ctx.end();

    // Check if reached the end of the range:
    if (it != end && *it != '}') {
      throw fmt::format_error("invalid format");
    }

    // Return an iterator past the end of the parsed range:
    return it;
  }

  template <typename FormatContext>
  auto format(const std::optional<T> &opt, FormatContext &ctx) const
      -> decltype(ctx.out()) {
    // ctx.out() is an output iterator to write to.

    if (opt.has_value()) {
      return fmt::format_to(ctx.out(), "{}", opt.value());
    }
    static constexpr std::string_view message("<none>");
    return std::copy(std::begin(message), std::end(message), ctx.out());
  }
};

template <typename T>
struct fmt::formatter<boost::optional<T>> {
  // Parses format specifications of the form ['s' | 'l'].
  constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin()) {
    // Parse the presentation format and store it in the formatter:
    auto it = ctx.begin(), end = ctx.end();

    // Check if reached the end of the range:
    if (it != end && *it != '}') {
      throw fmt::format_error("invalid format");
    }

    // Return an iterator past the end of the parsed range:
    return it;
  }

  // Formats the BlockInfo using the parsed format specification (presentation)
  // stored in this formatter.
  template <typename FormatContext>
  auto format(const boost::optional<T> &opt, FormatContext &ctx) const
      -> decltype(ctx.out()) {
    // ctx.out() is an output iterator to write to.

    if (opt.has_value()) {
      return fmt::format_to(ctx.out(), "{}", opt.value());
    }
    static constexpr std::string_view message("<none>");
    return std::copy(std::begin(message), std::end(message), ctx.out());
  }
};
