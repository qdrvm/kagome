/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "secure_mode_precheck.hpp"

#include <boost/asio/read.hpp>
#include <boost/asio/readable_pipe.hpp>
#include <boost/process/v2.hpp>
#include <boost/process/v2/environment.hpp>
#include <boost/process/v2/process.hpp>
#include <boost/process/v2/stdio.hpp>
#include <filesystem>
#include <iostream>
#include <libp2p/log/configurator.hpp>
#include <qtils/bytestr.hpp>
#include <scale/scale.hpp>
#include <soralog/macro.hpp>

#include "common/buffer.hpp"
#include "common/buffer_view.hpp"
#include "log/configurator.hpp"
#include "log/logger.hpp"
#include "parachain/pvf/secure_mode.hpp"
#include "utils/get_exe_path.hpp"

namespace kagome::parachain {

  SecureModeSupport checkSecureMode(
      const std::filesystem::path &original_cache_dir) {
#ifndef __linux__
    return SecureModeSupport::none();
#else
    std::filesystem::path cache_dir = original_cache_dir;
    SecureModeSupport support = SecureModeSupport::none();
    auto logger = log::createLogger("CheckSecureMode", "parachain");
    if (auto res = changeRoot(cache_dir)) {
      support.chroot = true;
    } else {
      SL_WARN(
          logger,
          "Secure mode incomplete, cannot change root directory to {} for PVF "
          "worker: {}",
          cache_dir.c_str(),
          res.error());
    }
    cache_dir = "/";

    if (auto res = enableLandlock(cache_dir)) {
      support.landlock = true;

    } else {
      SL_WARN(logger,
              "Secure mode incomplete, cannot enable landlock for PVF "
              "worker: {}",
              res.error());
    }
    if (auto res = enableSeccomp()) {
      support.seccomp = true;
    } else {
      SL_WARN(logger,
              "Secure mode incomplete, cannot enable seccomp for PVF "
              "worker: {}",
              res.error());
    }
    return support;
#endif
  }

  SecureModeOutcome<SecureModeSupport> runSecureModeCheckProcess(
      boost::asio::io_context &io_context,
      const std::filesystem::path &cache_dir) {
    namespace process_v2 = boost::process::v2;
    boost::asio::readable_pipe pipe{io_context};
    // input passed as CLI arguments to enable users to manually run the check
    process_v2::process process{io_context,
                                exePath().c_str(),
                                {"check-secure-mode", cache_dir.c_str()},
                                process_v2::process_stdio{{}, pipe, {}}};
    Buffer output;
    boost::system::error_code ec;
    boost::asio::read(pipe, boost::asio::dynamic_buffer(output), ec);

    if (process.wait() != 0) {
      return SecureModeError{"Secure mode check failed"};
    }
    OUTCOME_TRY(res, scale::decode<SecureModeSupport>(output));
    return res;
  }

  int secureModeCheckMain(int argc, const char **argv) {
    if (argc != 3) {
      if (argc < 3) {
        std::cerr << "Error: RUNTIME_CACHE_DIR parameter missing\n";
      } else if (argc > 3) {
        std::cerr << "Redundant arguments passed\n";
      }
      std::cerr << "Usage: kagome check-secure-mode RUNTIME_CACHE_DIR";
      return -1;
    }
    auto logging_system = std::make_shared<soralog::LoggingSystem>(
        std::make_shared<kagome::log::Configurator>(
            std::make_shared<libp2p::log::Configurator>()));
    auto r = logging_system->configure();
    if (not r.message.empty()) {
      std::cerr << r.message << '\n';
    }
    if (r.has_error) {
      return EXIT_FAILURE;
    }
    kagome::log::setLoggingSystem(logging_system);

    auto result = checkSecureMode(std::filesystem::path{argv[2]});
    auto enc_result = scale::encode(result);
    if (!enc_result) {
      std::cerr << "Failed to encode secure mode check result: "
                << enc_result.error().message() << "\n";
      return EXIT_FAILURE;
    }
    std::cout.write(qtils::byte2str(enc_result.value().data()),
                    enc_result.value().size());
    return 0;
  }

}  // namespace kagome::parachain
