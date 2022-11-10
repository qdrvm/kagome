/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_OUTCOME_HPP
#define KAGOME_OUTCOME_HPP

#include <fmt/format.h>
#include <libp2p/outcome/outcome.hpp>

namespace outcome {
  using libp2p::outcome::failure;
  using libp2p::outcome::result;
  using libp2p::outcome::success;
}  // namespace outcome

template <>
struct fmt::formatter<std::error_code> {
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

  // Formats the std::error_code
  template <typename FormatContext>
  auto format(const std::error_code &ec, FormatContext &ctx)
      -> decltype(ctx.out()) {
    // ctx.out() is an output iterator to write to.

    return format_to(ctx.out(), ec.message());
  }
};

template <typename Result, typename Failure>
struct fmt::formatter<outcome::result<Result, Failure>> {
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

  // Formats the outcome
  template <typename FormatContext>
  auto format(const outcome::result<Result, Failure> &res, FormatContext &ctx)
      -> decltype(ctx.out()) {
    // ctx.out() is an output iterator to write to.

    if (res.has_value()) {
      if constexpr (not std::is_void_v<Result>) {
        return format_to(ctx.out(), res.has_value());
      } else {
        return format_to(ctx.out(), "success");
      }
    } else {
      return format_to(ctx.out(), res.error().message());
    }
  }
};

#endif  // KAGOME_OUTCOME_HPP
