/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kagome_options.hpp"
//
#include <fstream>
#include <string>
//
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include "application/impl/config_reader/json_configuration_reader.hpp"

namespace kagome::options {
  using kagome::application::JsonConfigurationReader;
  using kagome::application::KagomeConfig;
  using kagome::application::KeyStorage;
  using kagome::application::LocalKeyStorage;

  // TODO (yuraz) get rid of default values after debug is finished
  const std::string local_path =
      boost::filesystem::path(__FILE__).parent_path().string() + "/debug";
  const std::string default_genesis_path = local_path + "/kagome_config.json";
  const std::string default_ed25519_path = local_path + "/ed25519key.pem";
  const std::string default_sr25519_path = local_path + "/sr25519key.txt";
  const std::string default_p2pkey_path = local_path + "/p2pkey.pem";

  KagomeOptions::KagomeOptions()
      : desc_("Kagome application allowed options") {}

  outcome::result<CmdLineOptionError> KagomeOptions::parseOptions(int argc,
                                                                  char **argv) {
    logger_->info("start parse options");

    // PARSE OPTIONS
    std::string configuration_path;  // configuration path
    std::string sr_path;             // path to sr25519 key pair
    std::string ed_path;             // path to ed25519 key pair
    std::string p2p_path;            // path to p2p key pair
    std::string p2p_type;            // p2p key type

    namespace po = boost::program_options;

    // clang-format off
  desc_.add_options()
      ("help,h", "show help message")(
      "config,c", po::value<std::string>(&configuration_path)->default_value(default_genesis_path),
      "path to configuration file")
      ("sr25519,s", po::value<std::string>(&sr_path)->default_value(default_sr25519_path),
       "mandatory, path to sr25519 keypair")
      ("ed25519,e", po::value<std::string>(&ed_path)->default_value(default_ed25519_path),
       "mandatory, path to ed25519 keypair")
      ("p2pkey,p", po::value<std::string>(&p2p_path)->default_value(default_p2pkey_path),
       "mandatory, path to p2p keypair")
      ("p2ptype,t", po::value<std::string>(&p2p_type)->default_value("ed25519"),
       "optional, type of p2p keypair, only following values are allowed: "
       "rsa, ed25519, secp256k1, ecdsa, default value is ed25519."
       "values are case-insensitive");
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

    // ENSURE THAT PATHS EXIST
    OUTCOME_TRY(ensurePathExists(configuration_path));
    OUTCOME_TRY(ensurePathExists(sr_path));
    OUTCOME_TRY(ensurePathExists(ed_path));
    OUTCOME_TRY(ensurePathExists(p2p_path));

    // READ KAGOME OPTIONS FROM FILE
    std::ifstream configuration_stream;
    configuration_stream.open(configuration_path);

    if (!configuration_stream.is_open()) {
      return outcome::failure(CmdLineOptionError::CANNOT_OPEN_FILE);
    }

    OUTCOME_TRY(kagome_config,
                JsonConfigurationReader::initConfig(configuration_stream));
    kagome_config_ = std::move(kagome_config);

    // CHECK P2P KEY TYPE
    auto &&p2p_type_result = parse_p2pType(p2p_type);
    if (!p2p_type_result) {
      logger_->error(p2p_type_result.error());
      return outcome::failure(CmdLineOptionError::UNSUPPORTED_P2P_KEY_TYPE);
    }

    // POPULATE KEYS CONFIG
    KeysConfig keys_config{
        {sr_path}, {ed_path}, {p2p_path}, p2p_type_result.value()};
    keys_config_ = std::move(keys_config);

    return outcome::success();
  }

  outcome::result<void> KagomeOptions::ensurePathExists(
      const boost::filesystem::path &path) {
    if (!boost::filesystem::exists(path)) {
      logger_->error("file path <%s> doesn't exist", path.string());
      return outcome::failure(CmdLineOptionError::CONFIG_FILE_NOT_EXIST);
    }
    return outcome::success();
  }

  outcome::result<libp2p::crypto::Key::Type> KagomeOptions::parse_p2pType(
      std::string_view type) {
    using KeyType = libp2p::crypto::Key::Type;

    if ("rsa" == type) {
      return KeyType::RSA;
    }
    if ("ed25519" == type) {
      return KeyType::Ed25519;
    }
    if ("secp256k1" == type) {
      return KeyType::Secp256k1;
    }
    if ("ecdsa" == type) {
      return KeyType::ECDSA;
    }
    return outcome::failure(CmdLineOptionError::UNSUPPORTED_P2P_KEY_TYPE);
  }

  const KagomeConfig &KagomeOptions::getKagomeConfig() const {
    return kagome_config_;
  }

  const KagomeOptions::KeysConfig &KagomeOptions::getKeysConfig() const {
    return keys_config_;
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
  }
  return "unknown CmdLineOptionError";
}
