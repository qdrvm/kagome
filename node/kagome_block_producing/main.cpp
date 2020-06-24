
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

  namespace po = boost::program_options;

  // clang-format off
	po::options_description desc("General options");
	desc.add_options()
			("help,h", "show this help message")
			("verbosity,v", po::value<int>(&verbosity)->default_value(2), "Log level: 0 - trace, 1 - debug, 2 - info, 3 - warn, 4 - error, 5 - crit, 6 - no log")
			;

	po::options_description blockhain_desc("Blockchain options");
	blockhain_desc.add_options()
			("genesis,g", po::value<std::string>(&configuration_path)->required(), "required, configuration file path")
			;

	po::options_description storage_desc("Storage options");
	storage_desc.add_options()
			("leveldb,l", po::value<std::string>(&leveldb_path)->required(), "required, leveldb directory path")
			;

	po::options_description authority_desc("Authority options");
	authority_desc.add_options()
			("keystore,k", po::value<std::string>(&keystore_path)->required(), "required, keystore file path")
			;

	po::options_description network_desc("Network options");
	network_desc.add_options()
			("p2p_port,p", po::value<uint16_t>(&p2p_port)->default_value(30363), "port for peer to peer interactions")
			("rpc_http_host", po::value<std::string>(&rpc_http_host)->default_value("0.0.0.0"), "address for RPC over HTTP")
			("rpc_http_port", po::value<uint16_t>(&rpc_http_port)->default_value(40363), "port for RPC over HTTP")
			("rpc_ws_host", po::value<std::string>(&rpc_ws_host)->default_value("0.0.0.0"), "address for RPC over Websocket protocol")
			("rpc_ws_port", po::value<uint16_t>(&rpc_ws_port)->default_value(40364), "port for RPC over Websocket protocol")
			;
  // clang-format on

  po::variables_map vm;
  po::parsed_options parsed = po::command_line_parser(argc, argv)
                                  .options(desc)
                                  .allow_unregistered()
                                  .run();
  po::store(parsed, vm);
  po::notify(vm);

  desc.add(blockhain_desc)
      .add(storage_desc)
      .add(authority_desc)
      .add(network_desc);

  if (vm.count("help") > 0) {
    std::cout << desc << std::endl;
    return EXIT_SUCCESS;
  }

  try {
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::store(parsed, vm);
    po::notify(vm);
  } catch (std::exception &e) {
    std::cerr << "Error: " << e.what() << '\n'
              << "Try run with option '--help' for more information"
              << std::endl;

    return EXIT_FAILURE;
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
          verbosity);
  app->run();

  return EXIT_SUCCESS;
}
