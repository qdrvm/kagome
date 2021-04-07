/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <iostream>

#include <libp2p/log/configurator.hpp>

#include "application/impl/app_configuration_impl.hpp"
#include "application/impl/validating_node_application.hpp"
#include "log/configurator.hpp"
#include "log/logger.hpp"

using kagome::application::AppConfiguration;
using kagome::application::AppConfigurationImpl;

int main(int argc, char **argv) {
  {
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

  auto logger = kagome::log::createLogger("AppConfiguration", "main");
  AppConfigurationImpl configuration{logger};

  if (configuration.initialize_from_args(
          AppConfiguration::LoadScheme::kValidating, argc, argv)) {
    auto &&app =
        std::make_shared<kagome::application::ValidatingNodeApplication>(
            configuration);

    kagome::log::setLevelOfGroup("main", configuration.verbosity());

    app->run();
  }

  return EXIT_SUCCESS;
}
