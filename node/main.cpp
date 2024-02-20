/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <iostream>

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
#include "utils/argv0.hpp"

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
      switch (*configuration->subcommand()) {
        using kagome::application::Subcommand;
        case Subcommand::ChainInfo:
          return app->chainInfo();
      }
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
                 "Run with `--help' argument to print usage"
              << std::endl;
  }

}  // namespace

int main(int argc, const char **argv) {
#if defined(BACKWARD_HAS_BACKTRACE)
  backward::SignalHandling sh;
#endif

  // Needed for zombienet
  setvbuf(stdout, nullptr, _IOLBF, 0);
  setvbuf(stderr, nullptr, _IOLBF, 0);
  kagome::argv0() = argv[0];

  if (argc > 1) {
    std::string_view name{argv[1]};
    if (name == "pvf-worker") {
      return kagome::parachain::pvf_worker_main(argc - 1, argv + 1);
    }
  }

  soralog::util::setThreadName("kagome");

  auto logging_system = std::make_shared<soralog::LoggingSystem>(
      std::make_shared<kagome::log::Configurator>(
          std::make_shared<libp2p::log::Configurator>()));

  auto r = logging_system->configure();
  if (not r.message.empty()) {
    (r.has_error ? std::cerr : std::cout) << r.message << std::endl;
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
      // The first argument is not subcommand, run as node
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
