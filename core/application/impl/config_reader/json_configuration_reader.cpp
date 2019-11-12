/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/impl/config_reader/json_configuration_reader.hpp"

#include <boost/filesystem.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "application/impl/config_reader/error.hpp"
#include "application/impl/config_reader/pt_util.hpp"

namespace kagome::application {
  namespace pt = boost::property_tree;

  outcome::result<KagomeConfig> JsonConfigurationReader::initConfig(
      std::istream &config_file_data) {
    OUTCOME_TRY(tree, readPropertyTree(config_file_data));
    return initConfigFromProperyTree(tree);
  }

  outcome::result<void> JsonConfigurationReader::updateConfig(
      KagomeConfig &config, std::istream &config_file_data) {
    OUTCOME_TRY(tree, readPropertyTree(config_file_data));
    OUTCOME_TRY(updateConfigFromProperyTree(config, tree));
    return outcome::success();
  }

  outcome::result<boost::property_tree::ptree>
  JsonConfigurationReader::readPropertyTree(std::istream &data) {
    pt::ptree tree;
    try {
      pt::read_json(data, tree);
    } catch (pt::json_parser_error &e) {
      return ConfigReaderError::PARSER_ERROR;
    }
    return tree;
  }

}  // namespace kagome::application
