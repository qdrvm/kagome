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
    uint16_t rpc_http_port;          // port for rpcs over HTTP
    uint16_t rpc_ws_port;            // port for rpcs over Websockets
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

    // ENSURE THAT PATHS EXIST
    OUTCOME_TRY(ensureFilePathExists(configuration_path));
    OUTCOME_TRY(ensureFilePathExists(keystore_path));

    key_storage_path_ = keystore_path;
    config_storage_path_ = configuration_path;
    leveldb_path_ = leveldb_path;
    p2p_port_ = p2p_port;
    rpc_http_port_ = rpc_http_port;
    rpc_ws_port_ = rpc_ws_port;
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

  uint16_t KagomeOptions::getRpcHttpPort() const {
    return rpc_http_port_;
  }

  uint16_t KagomeOptions::getRpcWsPort() const {
    return rpc_ws_port_;
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
  }
  return "unknown CmdLineOptionError";
}
