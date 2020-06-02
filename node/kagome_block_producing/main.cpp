
/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <boost/program_options.hpp>
#include <iostream>

#include "application/impl/block_producing_node_application.hpp"
#include "common/logger.hpp"

int main(int argc, char **argv) {
  auto logger = kagome::common::createLogger("Kagome block producing node: ");

  // PARSE OPTIONS
  std::string configuration_path;  // configuration file path
  std::string keystore_path;       // keystore file path
  std::string leveldb_path;        // leveldb directory path
  uint16_t p2p_port;               // port for peer to peer interactions
  std::string rpc_http_host;       // address for RPC over HTTP
  uint16_t rpc_http_port;          // port for RPC over HTTP
  std::string rpc_ws_host;         // address for RPC over Websocket protocol
  uint16_t rpc_ws_port;            // port for RPC over Websocket protocol
  int verbosity;  // log level (0-trace, 5-only critical, 6-no logs)
  bool is_genesis_epoch = false;  // if we need to execute genesis epoch

  namespace po = boost::program_options;
  po::options_description desc("Kagome block producing node allowed options");
  // clang-format off
  desc.add_options()
		("help,h", "show this help message")("genesis,g", po::value<std::string>(&configuration_path)->required(), "mandatory, configuration file path")
		("keystore,k", po::value<std::string>(&keystore_path)->required(), "mandatory, keystore file path")
		("leveldb,l", po::value<std::string>(&leveldb_path)->required(), "mandatory, leveldb directory path")
		("p2p_port,p", po::value<uint16_t>(&p2p_port)->default_value(30363), "port for peer to peer interactions")
		("rpc_http_host", po::value<std::string>(&rpc_http_host)->default_value("0.0.0.0"), "address for RPC over HTTP")
		("rpc_http_port", po::value<uint16_t>(&rpc_http_port)->default_value(40363), "port for RPC over HTTP")
		("rpc_ws_host", po::value<std::string>(&rpc_ws_host)->default_value("0.0.0.0"), "address for RPC over Websocket protocol")
		("rpc_ws_port", po::value<uint16_t>(&rpc_ws_port)->default_value(40364), "port for RPC over Websocket protocol")
		("verbosity,v", po::value<int>(&verbosity)->default_value(2), "Log level. 0 - trace, 1 - debug, 2 - info, 3 - warn, 4 - error, 5 - critical, 6 - no logs. Default: info")
		("genesis_epoch,e", "if we need to execute genesis epoch");
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

  if (vm.count("genesis_epoch")) {
    is_genesis_epoch = true;
  }

  auto ensureEndpointIsValid =
      [logger](const std::string &address,
               uint16_t port) -> boost::asio::ip::tcp::endpoint {
    boost::asio::ip::tcp::endpoint endpoint;
    boost::system::error_code err;

    endpoint.address(boost::asio::ip::address::from_string(address, err));
    if (err.failed()) {
      logger->error("RPC address '{}' is invalid", address);
      exit(EXIT_FAILURE);
    }

    if (port <= 0 || port >= 65535) {
      logger->error("RPC port '{}' is wrong", port);
      exit(EXIT_FAILURE);
    }
    endpoint.port(port);

    return endpoint;
  };

  auto rpc_http_endpoint = ensureEndpointIsValid(rpc_http_host, rpc_http_port);
  auto rpc_ws_endpoint = ensureEndpointIsValid(rpc_ws_host, rpc_ws_port);

  auto &&app =
      std::make_shared<kagome::application::BlockProducingNodeApplication>(
          configuration_path,
          keystore_path,
          leveldb_path,
          p2p_port,
          rpc_http_endpoint,
          rpc_ws_endpoint,
          is_genesis_epoch,
          verbosity);
  app->run();

  return 0;
}
