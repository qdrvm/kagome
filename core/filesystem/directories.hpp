/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
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
    if (!exists(path))
      if (!create_directories(path)) return false;

    return true;
  }

}  // namespace kagome::filesystem

#endif  // KAGOME_FILESYSTEM_DIRECTORIES_HPP
