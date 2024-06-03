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

  struct SecureModeSupport {
    SCALE_TIE(3);

    bool chroot;
    bool landlock;
    bool seccomp;

    static SecureModeSupport none() {
      return {false, false, false};
    };

    bool isTotallySupported() const {
      return chroot && landlock && seccomp;
    }
  };

  SecureModeSupport checkSecureMode(const std::filesystem::path &cache_dir);

  SecureModeOutcome<SecureModeSupport> runSecureModeCheckProcess(
      boost::asio::io_context &io_context,
      const std::filesystem::path &cache_dir);

  int secureModeCheckMain(int argc, const char **argv);

}  // namespace kagome::parachain