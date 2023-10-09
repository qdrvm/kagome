/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "filesystem/common.hpp"
#include "outcome/outcome.hpp"

namespace kagome::application::util {

  enum class Error {
    FAILED_TO_CREATE_DIR = 1,
    NOT_A_DIR,
  };

  outcome::result<void> init_directory(const filesystem::path &path);

}  // namespace kagome::application::util

OUTCOME_HPP_DECLARE_ERROR(kagome::application::util, Error);
