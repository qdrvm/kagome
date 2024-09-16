/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <fmt/format.h>

#include <functional>
#include <memory>

template <typename T>
struct fmt::formatter<std::reference_wrapper<T>> {
  // Parses format specifications. Must be empty
  constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin()) {
    // Parse the presentation format and store it in the formatter:
    auto it = ctx.begin(), end = ctx.end();

    // Check if reached the end of the range:
    if (it != end && *it != '}') {
      throw format_error("invalid format");
    }

    // Return an iterator past the end of the parsed range:
    return it;
  }

  // Formats the optional value
  template <typename FormatContext>
  auto format(const std::reference_wrapper<T> &ref, FormatContext &ctx) const
      -> decltype(ctx.out()) {
    // ctx.out() is an output iterator to write to.

    if (ref.has_value()) {
      return fmt::format_to(ctx.out(), "{}", ref.value());
    } else {
      static constexpr string_view message("<dangling>");
      return std::ranges::copy(message, ctx.out());
    }
  }
};

template <typename T>
struct fmt::formatter<std::shared_ptr<T>> {
  // Parses format specifications. Must be empty
  constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin()) {
    // Parse the presentation format and store it in the formatter:
    auto it = ctx.begin(), end = ctx.end();

    // Check if reached the end of the range:
    if (it != end && *it != '}') {
      throw format_error("invalid format");
    }

    // Return an iterator past the end of the parsed range:
    return it;
  }

  // Formats the optional value
  template <typename FormatContext>
  auto format(const std::shared_ptr<T> &sptr, FormatContext &ctx) const
      -> decltype(ctx.out()) {
    // ctx.out() is an output iterator to write to.

    if (sptr) {
      return fmt::format_to(ctx.out(), "{}", *sptr);
    } else {
      static constexpr string_view message("<nullptr>");
      return std::ranges::copy(message, ctx.out());
    }
  }
};

template <typename T>
struct fmt::formatter<std::unique_ptr<T>> {
  // Parses format specifications. Must be empty
  constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin()) {
    // Parse the presentation format and store it in the formatter:
    auto it = ctx.begin(), end = ctx.end();

    // Check if reached the end of the range:
    if (it != end && *it != '}') {
      throw format_error("invalid format");
    }

    // Return an iterator past the end of the parsed range:
    return it;
  }

  // Formats the optional value
  template <typename FormatContext>
  auto format(const std::unique_ptr<T> &uptr, FormatContext &ctx) const
      -> decltype(ctx.out()) {
    // ctx.out() is an output iterator to write to.

    if (uptr) {
      return fmt::format_to(ctx.out(), "{}", *uptr);
    } else {
      static constexpr string_view message("<nullptr>");
      return std::ranges::copy(message, ctx.out());
    }
  }
};

template <typename T>
struct fmt::formatter<std::weak_ptr<T>> {
  // Parses format specifications. Must be empty
  constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin()) {
    // Parse the presentation format and store it in the formatter:
    auto it = ctx.begin(), end = ctx.end();

    // Check if reached the end of the range:
    if (it != end && *it != '}') {
      throw format_error("invalid format");
    }

    // Return an iterator past the end of the parsed range:
    return it;
  }

  // Formats the optional value
  template <typename FormatContext>
  auto format(const std::weak_ptr<T> &wptr, FormatContext &ctx) const
      -> decltype(ctx.out()) {
    // ctx.out() is an output iterator to write to.

    if (not wptr.expired()) {
      return fmt::format_to(ctx.out(), "{}", *wptr.lock());
    } else {
      static constexpr string_view message("<expired>");
      return std::ranges::copy(message, ctx.out());
    }
  }
};
