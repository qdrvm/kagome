/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_FILESYSTEM_COMMON_HPP
#define KAGOME_FILESYSTEM_COMMON_HPP

#include <filesystem>

namespace kagome::filesystem {
  using namespace std::filesystem;

  path unique_path(const path &model = "%%%%-%%%%-%%%%-%%%%");
}  // namespace kagome::filesystem

#endif  // KAGOME_FILESYSTEM_COMMON_HPP
