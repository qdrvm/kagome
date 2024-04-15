/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>

namespace kagome {
  inline const std::string &exePath();
}  // namespace kagome

#ifdef __APPLE__
#include <mach-o/dyld.h>

const std::string &kagome::exePath() {
  static std::string path{_dyld_get_image_name(0)};
  return path;
}
#else
#include <filesystem>

const std::string &kagome::exePath() {
  static std::string path{std::filesystem::read_symlink("/proc/self/exe")};
  return path;
}
#endif
