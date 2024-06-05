/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <filesystem>

namespace kagome {
  inline const std::filesystem::path &exePath();
}  // namespace kagome

#ifdef __APPLE__
#include <mach-o/dyld.h>

const std::filesystem::path &kagome::exePath() {
  static const std::filesystem::path path{[] {
    std::string path;
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    path.resize(size);
    _NSGetExecutablePath(path.data(), &size);
    return path;
  }()};
  return path;
}
#else
const std::filesystem::path &kagome::exePath() {
  static const std::filesystem::path path{
      std::filesystem::read_symlink("/proc/self/exe")};
  return path;
}
#endif
