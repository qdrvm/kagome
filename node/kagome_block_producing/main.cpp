
/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <boost/program_options.hpp>
#include <iostream>

#include "application/impl/app_configuration_impl.hpp"
#include "application/impl/block_producing_node_application.hpp"
#include "common/logger.hpp"

using kagome::application::AppConfiguration;
using kagome::application::AppConfigurationImpl;

int main(int argc, char **argv) {
  auto logger = kagome::common::createLogger("Kagome block producing node: ");
  AppConfigurationImpl configuration{logger};

  if (configuration.initialize_from_args(
          AppConfiguration::LoadScheme::kBlockProducing, argc, argv)) {
    auto &&app =
        std::make_shared<kagome::application::BlockProducingNodeApplication>(
            configuration);
    app->run();
  }

  return EXIT_SUCCESS;
}
