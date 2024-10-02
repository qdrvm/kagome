/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <iostream>
#include "parachain/pvf/secure_mode_precheck.hpp"

#if defined(BACKWARD_HAS_BACKTRACE)
#include <backward.hpp>
#endif

#undef TRUE
#undef FALSE

#include <libp2p/common/final_action.hpp>
#include <libp2p/log/configurator.hpp>

#include "application/impl/app_configuration_impl.hpp"
#include "application/impl/kagome_application_impl.hpp"
#include "common/fd_limit.hpp"
#include "injector/application_injector.hpp"
#include "log/configurator.hpp"
#include "log/logger.hpp"
#include "parachain/pvf/kagome_pvf_worker.hpp"

// NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)

using kagome::application::AppConfiguration;
using kagome::application::AppConfigurationImpl;

int storage_explorer_main(int argc, const char **argv);
int db_editor_main(int argc, const char **argv);

namespace kagome {
  int benchmark_main(int argc, const char **argv);
}

namespace {
  int run_node(int argc, const char **argv) {
    auto configuration = std::make_shared<AppConfigurationImpl>();

    if (not configuration->initializeFromArgs(argc, argv)) {
      return EXIT_FAILURE;
    }

    kagome::log::tuneLoggingSystem(configuration->log());

    auto injector =
        std::make_unique<kagome::injector::KagomeNodeInjector>(configuration);

    kagome::common::setFdLimit(SIZE_MAX);

    auto app =
        std::make_shared<kagome::application::KagomeApplicationImpl>(*injector);

    if (configuration->subcommand().has_value()) {
      switch (configuration->subcommand().value()) {
        using kagome::application::Subcommand;
        case Subcommand::ChainInfo:
          return app->chainInfo();
      }
    }

    if (configuration->precompileWasm()) {
      return app->precompileWasm();
    }

    // Recovery mode
    if (configuration->recoverState().has_value()) {
      return app->recovery();
    }

    auto logger =
        kagome::log::createLogger("Main", kagome::log::defaultGroupName);

    SL_INFO(
        logger, "Kagome started. Version: {} ", configuration->nodeVersion());

    app->run();

    SL_INFO(logger, "Kagome stopped");
    logger->flush();

    return EXIT_SUCCESS;
  }

  void wrong_usage() {
    std::cerr << "Wrong usage.\n"
                 "Available subcommands: storage-explorer db-editor benchmark\n"
                 "Run with `--help' argument to print usage\n";
  }

}  // namespace

int main(int argc, const char **argv) {
#if defined(BACKWARD_HAS_BACKTRACE)
  backward::SignalHandling sh;
#endif

  // Needed for zombienet
  setvbuf(stdout, nullptr, _IOLBF, 0);
  setvbuf(stderr, nullptr, _IOLBF, 0);

  libp2p::common::FinalAction flush_std_streams_at_exit([] {
    std::cout.flush();
    std::cerr.flush();
  });

  if (argc > 1) {
    std::string_view name{argv[1]};
    if (name == "pvf-worker") {
      return kagome::parachain::pvf_worker_main(
          argc - 1, argv + 1, const_cast<const char **>(environ));
    }
    if (name == "check-secure-mode") {
      return kagome::parachain::secureModeCheckMain(argc, argv);
    }
  }

  soralog::util::setThreadName("kagome");

  // Logging system
  auto logging_system = [&] {
    auto custom_log_config_path =
        kagome::log::Configurator::getLogConfigFile(argc - 1, argv + 1);
    if (custom_log_config_path.has_value()) {
      if (not std::filesystem::is_regular_file(
              custom_log_config_path.value())) {
        std::cerr << "Provided wrong path to config file of logging\n";
        exit(EXIT_FAILURE);
      }
    }

    auto libp2p_log_configurator =
        std::make_shared<libp2p::log::Configurator>();

    auto kagome_log_configurator =
        custom_log_config_path.has_value()
            ? std::make_shared<kagome::log::Configurator>(
                  std::move(libp2p_log_configurator),
                  custom_log_config_path.value())
            : std::make_shared<kagome::log::Configurator>(
                  std::move(libp2p_log_configurator));

    return std::make_shared<soralog::LoggingSystem>(
        std::move(kagome_log_configurator));
  }();

  auto r = logging_system->configure();
  if (not r.message.empty()) {
    (r.has_error ? std::cerr : std::cout) << r.message << '\n';
  }
  if (r.has_error) {
    return EXIT_FAILURE;
  }

  kagome::log::setLoggingSystem(logging_system);

  int exit_code = EXIT_FAILURE;

  if (argc == 0) {
    // Abnormal run
    wrong_usage();
  }

  else if (argc == 1) {
    // Run without arguments
    wrong_usage();
  }

  else {
    std::string_view name{argv[1]};

    if (name == "storage-explorer") {
      exit_code = storage_explorer_main(argc - 1, argv + 1);
    }

    else if (name == "db-editor") {
      exit_code = db_editor_main(argc - 1, argv + 1);
    }

    else if (name == "benchmark") {
      exit_code = kagome::benchmark_main(argc - 1, argv + 1);
    }

    else if (name.substr(0, 1) == "-") {
      // The first argument isn't subcommand, run as node
      exit_code = run_node(argc, argv);

    } else {
      // No subcommand, but argument is not a valid option: begins not with dash
      wrong_usage();
    }
  }

  auto logger =
      kagome::log::createLogger("Main", kagome::log::defaultGroupName);
  SL_INFO(logger, "All components are stopped");
  logger->flush();

  return exit_code;
}

// NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)
