/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <filesystem>
#include <qtils/outcome.hpp>

namespace kagome {
  /**
   * Fix ambiguity of `create_directories` returning `bool` which doesn't
   * indicate error.
   */
  inline outcome::result<void> mkdirs(const std::filesystem::path &path) {
    std::error_code ec;
    std::filesystem::create_directories(path, ec);
    if (ec) {
      return ec;
    }
    return outcome::success();
  }
}  // namespace kagome
