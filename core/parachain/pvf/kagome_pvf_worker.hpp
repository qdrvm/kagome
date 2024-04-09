/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <filesystem>

#include "outcome/outcome.hpp"

namespace kagome::parachain {

  enum class SecureModeError {
    CHROOT_FAILED,
    ESCAPED_FROM_CHROOT,
    LANDLOCK_FAILED,
  };

  outcome::result<void> changeRoot(const std::filesystem::path &worker_dir);
  outcome::result<void> enableSeccomp();
  outcome::result<void> enableLandlock(const std::filesystem::path &worker_dir);

  int pvf_worker_main(int argc, const char **argv, const char **env);
}  // namespace kagome::parachain

OUTCOME_HPP_DECLARE_ERROR(kagome::parachain, SecureModeError);
