/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>

namespace kagome {
  /**
   * @returns String indicating current build version. Might to contain: tag,
   * number of commits from tag to fork, commit branch and number of commits
   * from fork to current commit.
   * @note Definition is generating by cmake
   */
  const std::string &buildVersion();
}  // namespace kagome
