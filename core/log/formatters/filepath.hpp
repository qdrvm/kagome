/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <fmt/core.h>
#include <filesystem>

template <>
struct fmt::formatter<std::filesystem::path> {
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
  auto format(const std::filesystem::path &path, FormatContext &ctx) const
      -> decltype(ctx.out()) {
    // ctx.out() is an output iterator to write to.

    auto native_path(path.native());
    return std::copy(std::begin(native_path), std::end(native_path), ctx.out());
  }
};
