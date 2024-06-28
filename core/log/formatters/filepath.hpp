/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <fmt/core.h>
#include <filesystem>

template <>
struct fmt::formatter<std::filesystem::path>
    : fmt::formatter<std::string_view> {
  auto format(const std::filesystem::path &path, format_context &ctx) {
    return fmt::formatter<std::string_view>::format(path.native(), ctx);
  }
};
