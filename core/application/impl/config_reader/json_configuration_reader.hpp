/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_JSON_CONFIGURATION_READER_HPP
#define KAGOME_JSON_CONFIGURATION_READER_HPP

#include "application/impl/kagome_config.hpp"

namespace kagome::application {

  /**
   * Reads a kagome configuration from a JSON file
   */
  class JsonConfigurationReader {
   public:
    /**
     * @param config_file path to the config file
     * @return kagome configuration if the file was correctly read and contained
     * the full config
     */
    static outcome::result<KagomeConfig> readFromFile(
        std::string_view config_file);

    /**
     * Updates parameters of config from entries  present in the file. In other
     * words, the config in the file may be not complete
     * @param config_file path to the config file
     * @return error if file couldn't be read or contained malformed content
     */
    static outcome::result<void> updateFromFile(KagomeConfig &config,
                                                std::string_view config_file);
  };
}  // namespace kagome::application

#endif  // KAGOME_JSON_CONFIGURATION_READER_HPP
