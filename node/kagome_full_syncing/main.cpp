/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <iostream>

#include <boost/program_options.hpp>
#include "application/impl/app_configuration_impl.hpp"
#include "application/impl/syncing_node_application.hpp"
#include "common/logger.hpp"

using kagome::application::AppConfiguration;
using kagome::application::AppConfigurationImpl;

int main(int argc, char **argv) {
  // TODO(xDimon): Use real logger. It's changed for probe
  //  auto logger = kagome::common::createLogger("Kagome full syncing node: ");
  auto logger = kagome::common::Logger();
  AppConfigurationImpl configuration{logger};

  if (configuration.initialize_from_args(
          AppConfiguration::LoadScheme::kFullSyncing, argc, argv)) {
    auto &&app = std::make_shared<kagome::application::SyncingNodeApplication>(
        configuration);
    app->run();
  }

  return EXIT_SUCCESS;
}
