/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/impl/kagome_application_impl.hpp"
#include "common/logger.hpp"
#include "kagome_options.hpp"
#include "outcome/outcome.hpp"

int main(int argc, char **argv) {
  auto logger = kagome::common::createLogger("Kagome full node: ");

  kagome::options::KagomeOptions options_parser;
  auto &&result = options_parser.parseOptions(argc, argv);
  if (!result) {
    logger->error(result.error().message());
    options_parser.showHelp();
    return 1;
  }

  if (options_parser.hasHelpOption()) {
    options_parser.showHelp();
    return 0;
  }

  auto &&kagome_config = options_parser.getKagomeConfigPath();
  auto &&keys_config = options_parser.getKeysConfig();
  auto &&level_db_config = options_parser.getLevelDbPath();

//  try {
    auto &&app = std::make_shared<kagome::application::KagomeApplicationImpl>(
        kagome_config, keys_config, level_db_config);
    app->run();
//  } catch (std::system_error &err) {
//    logger->error(err.what());
//    return 1;
//  } catch (std::exception &e) {
//    logger->error(e.what());
//    return 1;
//  } catch (...) {
//    logger->error("unknown error");
//    return 1;
//  }

  return 0;
}
