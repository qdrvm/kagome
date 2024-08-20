/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <filesystem>

#include "outcome/custom.hpp"

namespace kagome::parachain {
  struct SecureModeError : std::runtime_error {
    SecureModeError(const std::string &message) : std::runtime_error(message) {}

    SecureModeError(const std::error_code &ec)
        : SecureModeError{ec.message()} {}

    std::string_view message() const {
      return what();
    }
  };
  template <typename R>
  using SecureModeOutcome = CustomOutcome<R, SecureModeError>;

  inline auto format_as(const SecureModeError &e) {
    return e.message();
  }

  inline void outcome_throw_as_system_error_with_payload(SecureModeError e) {
    throw e;
  }

  /// Changes the filessystem root directory for the current process to
  /// worker_dir
  SecureModeOutcome<void> changeRoot(const std::filesystem::path &worker_dir);
  /// Prohibits network-related system calls
  SecureModeOutcome<void> enableSeccomp();
  /// Restricts access to the directories besides worker_dir
  SecureModeOutcome<void> enableLandlock(
      const std::filesystem::path &worker_dir);

}  // namespace kagome::parachain
