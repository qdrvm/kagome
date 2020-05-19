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
  auto p2p_port = options_parser.getP2PPort();
  auto rpc_http_endpoint = options_parser.getRpcHttpEndpoint();
  auto rpc_ws_endpoint = options_parser.getRpcWsEndpoint();
  auto verbosity = options_parser.getVerbosity();
  bool is_genesis_epoch = options_parser.isGenesisEpoch();

  auto &&app = std::make_shared<kagome::application::KagomeApplicationImpl>(
      kagome_config,
      keys_config,
      level_db_config,
      p2p_port,
      rpc_http_endpoint,
      rpc_ws_endpoint,
      is_genesis_epoch,
      verbosity);
  app->run();

  return 0;
}
