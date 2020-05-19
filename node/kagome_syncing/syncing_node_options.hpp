/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NODE_KAGOME_SYNCING_SYNCING_NODE_OPTIONS_HPP
#define KAGOME_NODE_KAGOME_SYNCING_SYNCING_NODE_OPTIONS_HPP

struct SyncingNodeOptions {
  boost::program_options::options_description desc_;  ///< options description
  bool has_help_ = false;  ///< flag whether cmd line has help option
  std::string key_storage_path_;
  std::string config_storage_path_;
  std::string leveldb_path_;
  uint16_t p2p_port_{};
  uint16_t rpc_http_endpoint_{};
  uint16_t rpc_ws_endpoint_{};
  uint8_t verbosity_{};
  bool is_genesis_epoch_{};

  /**
   * @brief parses options from command line
   * @param argc arguments count
   * @param argv arguments values
   * @return success if parsed successfully or error otherwise
   */
  outcome::result<CmdLineOptionError> parseOptions(int argc, char **argv);

  /**
   * @brief says whether cmd line has help option
   */
  bool hasHelpOption();

  /**
   * @brief prints help to stdout
   */
  void showHelp();
};

#endif  // KAGOME_NODE_KAGOME_SYNCING_SYNCING_NODE_OPTIONS_HPP
