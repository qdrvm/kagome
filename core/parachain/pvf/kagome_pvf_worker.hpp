/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <filesystem>

#include "outcome/outcome.hpp"

namespace kagome::parachain {

  struct SecureModeError {
    std::string_view message() const {
      return message_;
    }

    std::string message_;
  };

  outcome::result<void, SecureModeError> changeRoot(
      const std::filesystem::path &worker_dir);
  outcome::result<void, SecureModeError> enableSeccomp();
  outcome::result<void, SecureModeError> enableLandlock(
      const std::filesystem::path &worker_dir);

  int pvf_worker_main(int argc, const char **argv, const char **env);
}  // namespace kagome::parachain
