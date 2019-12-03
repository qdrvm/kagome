/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_EXAMPLES_KAGOME_FULL_KAGOME_OPTIONS_HPP
#define KAGOME_EXAMPLES_KAGOME_FULL_KAGOME_OPTIONS_HPP

#include <boost/program_options.hpp>
#include <libp2p/crypto/key.hpp>

#include "application/impl/kagome_config.hpp"
#include "application/impl/local_key_storage.hpp"
#include "common/logger.hpp"
#include "outcome/outcome.hpp"

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
  };
}  // namespace kagome::options

OUTCOME_HPP_DECLARE_ERROR(kagome::options, CmdLineOptionError)

namespace kagome::options {
  /**
   * @class KagomeOptions is designed to parse kagome application command line
   * options
   */
  class KagomeOptions {
    using KagomeConfig = application::KagomeConfig;
    using KeysConfig = application::LocalKeyStorage::Config;

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
    const KagomeConfig &getKagomeConfig() const;

    /**
     * @return  keys config
     */
    const KeysConfig &getKeysConfig() const;

   private:
    /**
     * @brief parse p2p type cmd line option
     * @param type type string value
     * @return type option
     */
    outcome::result<libp2p::crypto::Key::Type> parse_p2pType(
        std::string_view type);
    /**
     * @brief ensures that file path exists
     * @param path file path
     * @return success if exists, error otherwise
     */
    outcome::result<void> ensurePathExists(const boost::filesystem::path &path);

    boost::program_options::options_description desc_;  ///< options description
    bool has_help_ = false;         ///< flag whether cmd line has help option
    KagomeConfig kagome_config_{};  ///< kagome configuration
    KeysConfig keys_config_{};      ///< paths to keys
    common::Logger logger_ = common::createLogger("KagomeApplication");
  };
}  // namespace kagome::options

#endif  // KAGOME_EXAMPLES_KAGOME_FULL_KAGOME_OPTIONS_HPP
