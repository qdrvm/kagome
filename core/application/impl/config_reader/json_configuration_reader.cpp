/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/impl/config_reader/json_configuration_reader.hpp"

#include <boost/property_tree/json_parser.hpp>

#include "application/impl/config_reader/error.hpp"
#include "application/impl/config_reader/pt_util.hpp"

namespace kagome::application {
  namespace pt = boost::property_tree;

  outcome::result<KagomeConfig> JsonConfigurationReader::readFromFile(
      std::string_view config_file) {
    pt::ptree tree;
    try {
      pt::read_json(std::string(config_file), tree);
    } catch (pt::json_parser_error &) {
      return ConfigReaderError::FILE_NOT_FOUND;
    }
    return initConfigFromProperyTree(tree);
  }

  outcome::result<void> JsonConfigurationReader::updateFromFile(
      KagomeConfig &config, std::string_view config_file) {
    pt::ptree tree;
    try {
      pt::read_json(std::string(config_file), tree);
    } catch (pt::json_parser_error &) {
      return ConfigReaderError::FILE_NOT_FOUND;
    }
    OUTCOME_TRY(updateConfigFromProperyTree(config, tree));
    return outcome::success();
  }

}  // namespace kagome::application
