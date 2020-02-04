/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_EXAMPLES_KAGOME_FULL_KAGOME_OPTIONS_HPP
#define KAGOME_EXAMPLES_KAGOME_FULL_KAGOME_OPTIONS_HPP

#include <boost/program_options.hpp>
#include <libp2p/crypto/key.hpp>
#include <outcome/outcome.hpp>

#include "application/impl/local_key_storage.hpp"
#include "common/logger.hpp"

namespace kagome::options {
  /*
   * @brief parse command line options error enum
   */
  enum class CmdLineOptionError {
    UNSUPPORTED_P2P_KEY_TYPE = 1,  // p2p key type not supported
    INVALID_OPTIONS,               // invalid cmd line options
    CONFIG_FILE_NOT_EXIST,         // configuration file doesn't exist
    INVALID_CONFIG_FILE,           // failed to open configuration file
    CANNOT_OPEN_FILE,              // cannot open file
    LEVELDB_PATH_IS_NOT_DIR,       // leveldb path must be directory
  };
}  // namespace kagome::options

OUTCOME_HPP_DECLARE_ERROR(kagome::options, CmdLineOptionError)

namespace kagome::options {
  /**
   * @class KagomeOptions is designed to parse kagome application command line
   * options
   */
  class KagomeOptions {
   public:
    KagomeOptions();

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

    /**
     * @return  kagome config
     */
    const std::string &getKagomeConfigPath() const;

    /**
     * @return  keys config
     */
    const std::string &getKeysConfig() const;

    /**
     * @return leveldb directory path
     */
    const std::string &getLevelDbPath() const;

   private:
    /**
     * @brief ensures that file path exists
     * @param path file path
     * @return success if exists, error otherwise
     */
    outcome::result<void> ensureFilePathExists(
        const boost::filesystem::path &path);

    /**
     * @brief ensures that directory path exists
     * @param path file path
     * @return success if exists, error otherwise
     */
    outcome::result<void> ensureDirPathExists(
        const boost::filesystem::path &path);

    boost::program_options::options_description desc_;  ///< options description
    bool has_help_ = false;  ///< flag whether cmd line has help option
    std::string key_storage_path_;
    std::string config_storage_path_;
    std::string leveldb_path_;
    common::Logger logger_ = common::createLogger("Kagome options parser: ");
  };
}  // namespace kagome::options

#endif  // KAGOME_EXAMPLES_KAGOME_FULL_KAGOME_OPTIONS_HPP
