/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_JSON_CONFIGURATION_READER_HPP
#define KAGOME_JSON_CONFIGURATION_READER_HPP

#include <boost/property_tree/ptree.hpp>
#include "application/impl/kagome_config.hpp"

namespace kagome::application {

  /**
   * Reads a kagome configuration from a JSON file
   */
  class JsonConfigurationReader {
   public:
    /**
     * @param config_file_data stream with the config file data
     * @return kagome configuration if the data was correctly read and contained
     * the full config
     */
    static outcome::result<KagomeConfig> initConfig(
        std::istream &config_file_data);

    /**
     * Updates parameters of config from entries present in the config data. In
     * other words, the config in the stream may be incomplete
     * @param config_file_data stream with the config file data
     * @return error if the stream couldn't be read or contained malformed
     * content
     */
    static outcome::result<void> updateConfig(KagomeConfig &config,
                                              std::istream &config_file_data);

   private:
    static outcome::result<boost::property_tree::ptree> readPropertyTree(
        std::istream &data);
  };
}  // namespace kagome::application

#endif  // KAGOME_JSON_CONFIGURATION_READER_HPP
