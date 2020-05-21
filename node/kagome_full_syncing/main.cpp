/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <iostream>

#include <boost/program_options.hpp>
#include "application/impl/syncing_node_application.hpp"
#include "common/logger.hpp"

int main(int argc, char **argv) {
  auto logger = kagome::common::createLogger("Kagome full node: ");

  // PARSE OPTIONS
  std::string configuration_path;  // configuration file path
  std::string leveldb_path;        // leveldb directory path
  uint16_t p2p_port;               // port for peer to peer interactions
  uint16_t rpc_http_port;          // port for rpcs over HTTP
  uint16_t rpc_ws_port;            // port for rpcs over Websockets
  int verbosity;  // log level (0-trace, 5-only critical, 6-no logs)

  namespace po = boost::program_options;
  po::options_description desc("Kagome syncing node allowed options");
  // clang-format off
  desc.add_options()
      ("help,h", "show this help message")(
      "genesis,g", po::value<std::string>(&configuration_path)->required(),
      "mandatory, configuration file path")
      ("leveldb,l", po::value<std::string>(&leveldb_path)->required(),
       "mandatory, leveldb directory path")
      ("p2p_port,p", po::value<uint16_t>(&p2p_port)->default_value(30363),
       "port for peer to peer interactions")
      ("rpc_http_port", po::value<uint16_t>(&rpc_http_port)->default_value(40363),
       "port for RPCs over HTTP")
      ("rpc_ws_port", po::value<uint16_t>(&rpc_ws_port)->default_value(40364),
       "port for RPCs over Websockets")
      ("genesis_epoch,e", "if we need to execute genesis epoch")
      ("verbosity,v", po::value<int>(&verbosity)->default_value(2),
       "Log level. 0 - trace, 1 - debug, 2 - info, 3 - warn, 4 - error, 5 - critical, 6 - no logs. Default: info");
  // clang-format on

  po::variables_map vm;
  try {
    po::store(po::command_line_parser(argc, argv).options(desc).run(), vm);
    po::notify(vm);
  } catch (std::exception &e) {
    logger->error(e.what());
    return 1;
  }

  if (vm.count("help") > 0) {
    std::cout << desc << std::endl;
    return 0;
  }

  auto &&app = std::make_shared<kagome::application::SyncingNodeApplication>(
      configuration_path,
      leveldb_path,
      p2p_port,
      rpc_http_port,
      rpc_ws_port,
      verbosity);
  app->run();

  return 0;
}
