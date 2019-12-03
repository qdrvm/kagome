/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/impl/kagome_application_impl.hpp"
#include "application/impl/local_key_storage.hpp"
#include "common/logger.hpp"
#include "kagome_options.hpp"
#include "outcome/outcome.hpp"

int main(int argc, char **argv) {
  auto logger = kagome::common::createLogger("Kagome full node");

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

  try {
    auto &&app = std::make_shared<kagome::application::KagomeApplicationImpl>(
        std::move(kagome_config), std::move(keys_config));
    app->run();
  } catch (std::system_error &err) {
    std::cerr << err.what() << std::endl;
    return 1;
  } catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
    return 1;
  } catch (...) {
    std::cerr << "unknown error" << std::endl;
    return 1;
  }

  return 0;
}
