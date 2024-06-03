/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/asio/io_context.hpp>
#include <filesystem>

#include "parachain/pvf/secure_mode.hpp"
#include "qtils/outcome.hpp"
#include "scale/tie.hpp"

namespace kagome::parachain {

  /**
   * Describes which parts of secure validator mode are supported by the current
   * platform
   */
  struct SecureModeSupport {
    SCALE_TIE(3);

    // The filesystem root of the PVF process can be set to the worker directory
    bool chroot;

    // Landlock can be enabled for the worker directory
    bool landlock;

    // Seccomp can be enabled to restrict access to syscalls for the worker
    // process
    bool seccomp;

    static SecureModeSupport none() {
      return {false, false, false};
    };

    bool isTotallySupported() const {
      return chroot && landlock && seccomp;
    }
  };

  /**
   * Attempts to enable secure validator mode, reporting which parts succeeded.
   * Meant to be executed in a disposable child process.
   */
  SecureModeSupport checkSecureMode(const std::filesystem::path &cache_dir);

  /**
   * Spawns a child process that executes checkSecureMode
   */
  SecureModeOutcome<SecureModeSupport> runSecureModeCheckProcess(
      boost::asio::io_context &io_context,
      const std::filesystem::path &cache_dir);

  /**
   * Main entry for a child process that executes checkSecureMode
   */
  int secureModeCheckMain(int argc, const char **argv);

}  // namespace kagome::parachain
