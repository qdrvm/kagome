/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <filesystem>

namespace kagome::filesystem {
  using namespace std::filesystem;  // NOLINT(google-build-using-namespace)

  path unique_path(const path &model = "%%%%-%%%%-%%%%-%%%%");
}  // namespace kagome::filesystem
