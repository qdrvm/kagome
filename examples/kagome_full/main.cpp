/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
  auto rpc_http_port = options_parser.getRpcHttpPort();
  auto rpc_ws_port = options_parser.getRpcWsPort();
  auto verbosity = options_parser.getVerbosity();
  bool is_genesis_epoch = options_parser.isGenesisEpoch();

  auto &&app = std::make_shared<kagome::application::KagomeApplicationImpl>(
      kagome_config,
      keys_config,
      level_db_config,
      p2p_port,
      rpc_http_port,
      rpc_ws_port,
      is_genesis_epoch,
      verbosity);
  app->run();

  return 0;
}
