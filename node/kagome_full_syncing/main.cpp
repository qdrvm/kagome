/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <iostream>

#include <boost/program_options.hpp>
#include "application/impl/app_config_impl.hpp"
#include "application/impl/syncing_node_application.hpp"
#include "common/logger.hpp"

using kagome::application::AppConfiguration;
using kagome::application::AppConfigurationImpl;

int main(int argc, char **argv) {
  auto logger = kagome::common::createLogger("Kagome full node: ");
  auto configuration = std::make_shared<AppConfigurationImpl>(logger);

  try {
    configuration->initialize_from_args(
        AppConfiguration::LoadScheme::kFullSyncing, argc, argv);

    auto &&app = std::make_shared<kagome::application::SyncingNodeApplication>(
        std::move(configuration));
    app->run();
  } catch (std::exception &e) {
    logger->error("Caught exception: {}", e.what());
  } catch (...) {
    logger->error("Caught exception");
  }
  return EXIT_SUCCESS;
}
