/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_FILESYSTEM_DIRECTORIES_HPP
#define KAGOME_FILESYSTEM_DIRECTORIES_HPP

#include "filesystem/common.hpp"

namespace kagome::filesystem {

  /**
   * Creates directories recursively
   * @param path full path or relative path to directory to be created
   * @return true if path exists, otherwise false
   */
  inline bool createDirectoryRecursive(const path &path) {
    return exists(path) || create_directories(path);
  }

}  // namespace kagome::filesystem

#endif  // KAGOME_FILESYSTEM_DIRECTORIES_HPP
