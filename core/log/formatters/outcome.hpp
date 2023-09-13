/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <fmt/core.h>
#include <boost/system.hpp>

// Remove after it will be added to libp2p (will be happened compilation error)
template <>
struct ::fmt::formatter<boost::system::error_code> {
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

  // Formats the boost::system::error_code
  template <typename FormatContext>
  auto format(const boost::system::error_code &ec, FormatContext &ctx) const
      -> decltype(ctx.out()) {
    const auto &message = ec.message();
    return std::copy(std::begin(message), std::end(message), ctx.out());
  }
};

// Remove after it will be added to libp2p (will be happened compilation error)
template <typename T>
struct ::fmt::formatter<libp2p::outcome::success_type<T>> {
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

  // Formats the success<non_void_type>
  template <typename OutputIt>
  typename std::enable_if_t<not std::is_void_v<T>, OutputIt> format_impl(
      OutputIt out, const libp2p::outcome::success_type<T> &success) const {
    return ::fmt::format_to(out, "{}", success.value());
  }

  // Formats the success<void>
  template <typename OutputIt>
  typename std::enable_if_t<std::is_void_v<T>, OutputIt> format_impl(
      OutputIt out, const libp2p::outcome::success_type<void> &) const {
    static constexpr string_view message("<success>");
    return std::copy(std::begin(message), std::end(message), out);
  }

  // Formats the success<T>
  template <typename FormatContext>
  auto format(const libp2p::outcome::success_type<T> &success,
              FormatContext &ctx) const -> decltype(ctx.out()) {
    return format_impl(ctx.out(), success);
  }
};
