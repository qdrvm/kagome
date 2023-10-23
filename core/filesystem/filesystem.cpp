/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "filesystem/common.hpp"

#include <boost/filesystem.hpp>

namespace kagome::filesystem {

  path unique_path(const path &model) {
    return boost::filesystem::unique_path(model.c_str()).c_str();
  }

}  // namespace kagome::filesystem
