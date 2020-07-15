/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <iostream>

#include <boost/program_options.hpp>
#include "application/impl/app_config_impl.hpp"
#include "application/impl/validating_node_application.hpp"
#include "common/logger.hpp"
#include "outcome/outcome.hpp"

using kagome::application::AppConfiguration;
using kagome::application::AppConfigurationImpl;

int main(int argc, char **argv) {
  auto logger = kagome::common::createLogger("Kagome block producing node: ");
  auto configuration = std::make_shared<AppConfigurationImpl>(logger);

  configuration->initialize_from_args(
      AppConfiguration::LoadScheme::kValidating, argc, argv);
  auto &&app = std::make_shared<kagome::application::ValidatingNodeApplication>(
      std::move(configuration));
  app->run();

  return EXIT_SUCCESS;
}
