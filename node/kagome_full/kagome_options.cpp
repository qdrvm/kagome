/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kagome_options.hpp"

#include <fstream>
#include <string>

#include <boost/filesystem.hpp>

namespace kagome::options {
  using kagome::application::KeyStorage;
  using kagome::application::LocalKeyStorage;

  KagomeOptions::KagomeOptions()
      : desc_("Kagome application allowed options") {}

  outcome::result<CmdLineOptionError> KagomeOptions::parseOptions(int argc,
                                                                  char **argv) {
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
    is_genesis_epoch_ = false;  // if we need to execute genesis epoch

    namespace po = boost::program_options;

    // clang-format off
  desc_.add_options()
      ("help,h", "show this help message")(
      "genesis,g", po::value<std::string>(&configuration_path)->required(),
      "mandatory, configuration file path")
      ("keystore,k", po::value<std::string>(&keystore_path)->required(),
       "mandatory, keystore file path")
      ("leveldb,l", po::value<std::string>(&leveldb_path)->required(),
          "mandatory, leveldb directory path")
      ("p2p_port,p", po::value<uint16_t>(&p2p_port)->default_value(defaultP2pPort),
       "port for peer to peer interactions")

      ("rpc_http_host", po::value<std::string>(&rpc_http_host)->default_value("0.0.0.0"), "address for RPC over HTTP")
      ("rpc_http_port", po::value<uint16_t>(&rpc_http_port)->default_value(defaultRpcHttpPort), "port for RPC over HTTP")
      ("rpc_ws_host", po::value<std::string>(&rpc_ws_host)->default_value("0.0.0.0"), "address for RPC over Websocket protocol")
      ("rpc_ws_port", po::value<uint16_t>(&rpc_ws_port)->default_value(defaultRpcWebsockerPort), "port for RPC over Websocket protocol")

      ("genesis_epoch,e", "if we need to execute genesis epoch")
      ("verbosity,v", po::value<int>(&verbosity)->default_value(2),
       "Log level. 0 - trace, 1 - debug, 2 - info, 3 - warn, 4 - error, 5 - critical, 6 - no logs. Default: info");
    // clang-format on

    po::variables_map vm;
    try {
      po::store(po::command_line_parser(argc, argv).options(desc_).run(), vm);
      po::notify(vm);
    } catch (std::exception &e) {
      logger_->error(e.what());
      return outcome::failure(CmdLineOptionError::INVALID_OPTIONS);
    }

    if (vm.count("help") > 0) {
      has_help_ = true;
    }

    if (vm.count("genesis_epoch")) {
      is_genesis_epoch_ = true;
    }

    OUTCOME_TRY(ensureFilePathExists(configuration_path));
    OUTCOME_TRY(ensureFilePathExists(keystore_path));

    OUTCOME_TRY(rpc_http_endpoint,
                ensureEndpointIsValid(rpc_http_host, rpc_http_port));
    OUTCOME_TRY(rpc_ws_endpoint,
                ensureEndpointIsValid(rpc_ws_host, rpc_ws_port));

    key_storage_path_ = keystore_path;
    config_storage_path_ = configuration_path;
    leveldb_path_ = leveldb_path;
    p2p_port_ = p2p_port;
    rpc_http_endpoint_ = rpc_http_endpoint;
    rpc_ws_endpoint_ = rpc_ws_endpoint;
    verbosity_ = verbosity;

    return outcome::success();
  }

  outcome::result<void> KagomeOptions::ensureFilePathExists(
      const boost::filesystem::path &path) {
    if (!boost::filesystem::exists(path)) {
      logger_->error("file path '{}' doesn't exist", path.string());
      return outcome::failure(CmdLineOptionError::CONFIG_FILE_NOT_EXIST);
    }
    return outcome::success();
  }

  outcome::result<void> KagomeOptions::ensureDirPathExists(
      const boost::filesystem::path &path) {
    if (!boost::filesystem::exists(path)) {
      logger_->error("directory path '{}' doesn't exist", path.string());
      return outcome::failure(CmdLineOptionError::CONFIG_FILE_NOT_EXIST);
    }
    if (!boost::filesystem::is_directory(path)) {
      logger_->error("leveldb path '{}' is not directory", path.string());
      return CmdLineOptionError::LEVELDB_PATH_IS_NOT_DIR;
    }

    return outcome::success();
  }

  outcome::result<KagomeOptions::Endpoint> KagomeOptions::ensureEndpointIsValid(
      const std::string &address, uint16_t port) {
    Endpoint endpoint;
    boost::system::error_code err;

    endpoint.address(boost::asio::ip::address::from_string(address, err));
    if (err.failed()) {
      logger_->error("address '{}' is invalid", address);
      return outcome::failure(CmdLineOptionError::INVALID_ENDPOINT);
    }

    if (port <= 0 || port >= 65535) {
      logger_->error("port '{}' is wrong", port);
      return outcome::failure(CmdLineOptionError::INVALID_ENDPOINT);
    }
    endpoint.port(port);

    return endpoint;
  }

  const std::string &KagomeOptions::getKagomeConfigPath() const {
    return config_storage_path_;
  }

  const std::string &KagomeOptions::getKeysConfig() const {
    return key_storage_path_;
  }

  const std::string &KagomeOptions::getLevelDbPath() const {
    return leveldb_path_;
  }

  uint16_t KagomeOptions::getP2PPort() const {
    return p2p_port_;
  }

  const KagomeOptions::Endpoint &KagomeOptions::getRpcHttpEndpoint() const {
    return rpc_http_endpoint_;
  }

  const KagomeOptions::Endpoint &KagomeOptions::getRpcWsEndpoint() const {
    return rpc_ws_endpoint_;
  }

  uint8_t KagomeOptions::getVerbosity() const {
    return verbosity_;
  }

  bool KagomeOptions::isGenesisEpoch() const {
    return is_genesis_epoch_;
  }

  bool KagomeOptions::hasHelpOption() {
    return has_help_;
  }

  void KagomeOptions::showHelp() {
    std::cout << desc_ << std::endl;
  }
}  // namespace kagome::options

OUTCOME_CPP_DEFINE_CATEGORY(kagome::options, CmdLineOptionError, e) {
  using kagome::options::CmdLineOptionError;
  switch (e) {
    case CmdLineOptionError::UNSUPPORTED_P2P_KEY_TYPE:
      return "p2p key type is not supported";
    case CmdLineOptionError::INVALID_OPTIONS:
      return "invalid command line options";
    case CmdLineOptionError::CONFIG_FILE_NOT_EXIST:
      return "configuration file doesn't exist";
    case CmdLineOptionError::INVALID_CONFIG_FILE:
      return "invalid configuration file";
    case CmdLineOptionError::CANNOT_OPEN_FILE:
      return "failed to open configuration file";
    case CmdLineOptionError::LEVELDB_PATH_IS_NOT_DIR:
      return "leveldb path is not a directory";
    case CmdLineOptionError::INVALID_ENDPOINT:
      return "invalid endpoint";
  }
  return "unknown CmdLineOptionError";
}
