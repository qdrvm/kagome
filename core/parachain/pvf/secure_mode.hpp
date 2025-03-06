/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <filesystem>

#include "outcome/custom.hpp"

namespace kagome::parachain {
  struct SecureModeError {
    SecureModeError(std::string message) : message_{std::move(message)} {}

    SecureModeError(const std::error_code &ec) : message_{ec.message()} {}

    std::string_view message() const {
      return message_;
    }

    std::string message_;
  };

  inline auto make_exception_ptr(SecureModeError e) {
    return std::make_exception_ptr(std::move(e));
  }

  template <typename R>
  using SecureModeOutcome = CustomOutcome<R, SecureModeError>;

  inline auto format_as(const SecureModeError &e) {
    return e.message();
  }

  /// Changes the filesystem root directory for the current process to
  /// worker_dir
  SecureModeOutcome<void> changeRoot(const std::filesystem::path &worker_dir);
  /// Prohibits network-related system calls
  SecureModeOutcome<void> enableSeccomp();
  /// Restricts access to the directories besides worker_dir
  SecureModeOutcome<void> enableLandlock(
      const std::filesystem::path &worker_dir);

}  // namespace kagome::parachain
