/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/impl/config_reader/json_configuration_reader.hpp"

#include <gtest/gtest.h>

#include "application/impl/config_reader/error.hpp"
#include "kagome_configuration/example_config.hpp"
#include "testutil/outcome.hpp"

using kagome::application::ConfigReaderError;
using kagome::application::getExampleConfig;
using kagome::application::JsonConfigurationReader;
using kagome::application::KagomeConfig;
using kagome::application::readJSONConfig;

/**
 * @given a json file with configuration
 * @when initialising configuration storage from this file
 * @then the content of the storage matches the content of the file
 */
TEST(JsonConfigReader, LoadConfig) {
  auto ss = readJSONConfig();
  EXPECT_OUTCOME_TRUE(config, JsonConfigurationReader::initConfig(ss));
  ASSERT_EQ(config, getExampleConfig());
}

/**
 * @given a json file with configuration
 * @when updating configuration storage from this file
 * @then the content of the storage matches the content of the file
 */
TEST(JsonConfigReader, UpdateConfig) {
  KagomeConfig config = getExampleConfig();
  config.genesis.header.number = 34;        // 42 in the config
  config.api_ports.extrinsic_api_port = 0;  // 4224 in the config
  config.peers_info.clear();
  config.authorities.clear();
  config.session_keys.clear();
  auto ss = readJSONConfig();
  // read INIT_NUMBER back from JSON
  EXPECT_OUTCOME_TRUE_1(JsonConfigurationReader::updateConfig(config, ss));
  ASSERT_EQ(config, getExampleConfig());
}

/**
 * @given a json file with malformed content
 * @when reading configuration from this file
 * @then parser error is returned
 */
TEST(JsonConfigReader, ParserError) {
  std::stringstream config_data;
  config_data << "{\n";
  config_data << "\t\"genesis: \"0000\"\n";
  config_data << "}\n";
  EXPECT_OUTCOME_FALSE(e, JsonConfigurationReader::initConfig(config_data));
  ASSERT_EQ(e, ConfigReaderError::PARSER_ERROR);
}

/**
 * @given a json file with incomplete config
 * @when reading configuration from this file
 * @then missing entry error is returned
 */
TEST(JsonConfigReader, MissingEntry) {
  std::stringstream config_data;
  config_data << "{\n";
  config_data << "}\n";
  EXPECT_OUTCOME_FALSE(e, JsonConfigurationReader::initConfig(config_data));
  ASSERT_EQ(e, ConfigReaderError::MISSING_ENTRY);
}
