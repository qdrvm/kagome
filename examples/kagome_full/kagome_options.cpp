/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kagome_options.hpp"

#include <fstream>
#include <string>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

namespace kagome::options {
  using kagome::application::KagomeConfig;
  using kagome::application::KeyStorage;
  using kagome::application::LocalKeyStorage;

  KagomeOptions::KagomeOptions()
      : desc_("Kagome application allowed options") {}

  outcome::result<CmdLineOptionError> KagomeOptions::parseOptions(int argc,
                                                                  char **argv) {
    logger_->info("start parse options");

    // PARSE OPTIONS
    std::string configuration_path;  // configuration path
    std::string keystore_path;       // path to keystore

    namespace po = boost::program_options;

    // clang-format off
  desc_.add_options()
      ("help,h", "show help message")(
      "config,c", po::value<std::string>(&configuration_path),
      "path to configuration file")
      ("keystore,k", po::value<std::string>(&keystore_path),
       "mandatory, path to keystore");
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
    OUTCOME_TRY(ensurePathExists(keystore_path));

    key_storage_path_ = keystore_path;
    config_storage_path_ = configuration_path;

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

  const std::string &KagomeOptions::getKagomeConfigPath() const {
    return config_storage_path_;
  }

  const std::string &KagomeOptions::getKeysConfig() const {
    return key_storage_path_;
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
