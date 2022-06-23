/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <iostream>

#include <backward.hpp>
#undef TRUE
#undef FALSE
#include <libp2p/log/configurator.hpp>

#include "application/impl/app_configuration_impl.hpp"
#include "application/impl/kagome_application_impl.hpp"
#include "log/configurator.hpp"
#include "log/logger.hpp"

using kagome::application::AppConfiguration;
using kagome::application::AppConfigurationImpl;

int main(int argc, const char **argv) {
  backward::SignalHandling sh;

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
