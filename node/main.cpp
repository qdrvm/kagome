/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <iostream>

#if defined(BACKWARD_HAS_BACKTRACE)
#include <backward.hpp>
#endif

#undef TRUE
#undef FALSE

#include <libp2p/log/configurator.hpp>

#include "application/impl/app_configuration_impl.hpp"
#include "application/impl/kagome_application_impl.hpp"
#include "common/fd_limit.hpp"
#include "log/configurator.hpp"
#include "log/logger.hpp"

using kagome::application::AppConfiguration;
using kagome::application::AppConfigurationImpl;

int storage_explorer_main(int argc, const char **argv);
int db_editor_main(int argc, const char **argv);

int main(int argc, const char **argv) {
  if (argc > 1) {
    std::string_view name{argv[1]};
    if (name == "storage-explorer") {
      return storage_explorer_main(argc, argv + 1);
    }
    if (name == "db-editor") {
      return db_editor_main(argc, argv + 1);
    }
  }

#if defined(BACKWARD_HAS_BACKTRACE)
  backward::SignalHandling sh;
#endif
  {
    soralog::util::setThreadName("kagome");

    auto logging_system = std::make_shared<soralog::LoggingSystem>(
        std::make_shared<kagome::log::Configurator>(
            std::make_shared<libp2p::log::Configurator>()));

    auto r = logging_system->configure();
    if (not r.message.empty()) {
      (r.has_error ? std::cerr : std::cout) << r.message << std::endl;
    }
    if (r.has_error) {
      exit(EXIT_FAILURE);
    }

    kagome::log::setLoggingSystem(logging_system);
  }

  auto logger = kagome::log::createLogger("AppConfiguration",
                                          kagome::log::defaultGroupName);
  AppConfigurationImpl configuration{logger};

  kagome::common::setFdLimit(SIZE_MAX);

  if (configuration.initializeFromArgs(argc, argv)) {
    kagome::log::tuneLoggingSystem(configuration.log());

    auto app = std::make_shared<kagome::application::KagomeApplicationImpl>(
        configuration);

    if (configuration.subcommandChainInfo()) {
      return app->chainInfo();
    }

    // Recovery mode
    if (configuration.recoverState().has_value()) {
      return app->recovery();
    }

    app->run();
  }

  return EXIT_SUCCESS;
}
