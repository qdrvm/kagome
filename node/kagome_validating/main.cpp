/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <iostream>

#include <boost/program_options.hpp>
#include "application/impl/app_configuration_impl.hpp"
#include "application/impl/validating_node_application.hpp"
#include "log/logger.hpp"
#include "outcome/outcome.hpp"

using kagome::application::AppConfiguration;
using kagome::application::AppConfigurationImpl;

int main(int argc, char **argv) {
  // TODO(xDimon): Use real logger. It's changed for probe
  //  auto logger = kagome::log::createLogger("Kagome block producing and validating node: ");
  auto logger = kagome::log::Logger();
  AppConfigurationImpl configuration{logger};

  if (configuration.initialize_from_args(
          AppConfiguration::LoadScheme::kValidating, argc, argv)) {
    auto &&app =
        std::make_shared<kagome::application::ValidatingNodeApplication>(
            configuration);
    app->run();
  }

  return EXIT_SUCCESS;
}
