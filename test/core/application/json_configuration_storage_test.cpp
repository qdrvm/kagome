/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <scale/scale.hpp>

#include "application/impl/configuration_storage_impl.hpp"
#include "application/impl/config_reader/json_configuration_reader.hpp"
#include "primitives/block.hpp"
#include "boost/filesystem.hpp"
#include "testutil/outcome.hpp"

using kagome::application::ConfigurationStorageImpl;
using kagome::application::JsonConfigurationReader;
using kagome::application::KagomeConfig;

/**
 * @given a json file with configuration
 * @when initialising configuration storage from this file
 * @then the content of the storage matches the content of the file
 */
TEST(JsonConfigStorage, LoadConfig) {
  auto config_file_path =
      boost::filesystem::path(__FILE__).parent_path().string()
      + "/test_config.json";
  EXPECT_OUTCOME_TRUE(config, JsonConfigurationReader::readFromFile(config_file_path));
  auto s = ConfigurationStorageImpl(std::move(config));
  ASSERT_EQ(s.getGenesis(), kagome::primitives::Block());
}

/**
 * @given a json file with configuration
 * @when updating configuration storage from this file
 * @then the content of the storage matches the content of the file
 */
TEST(JsonConfigStorage, UpdateConfig) {
  auto config_file_path =
      boost::filesystem::path(__FILE__).parent_path().string()
      + "/test_config.json";
  KagomeConfig config;
  config.genesis = kagome::primitives::Block();
  config.genesis.header.number = 42;
  EXPECT_OUTCOME_TRUE_1(JsonConfigurationReader::updateFromFile(config, config_file_path));
  auto s = ConfigurationStorageImpl(std::move(config));
  ASSERT_EQ(s.getGenesis(), kagome::primitives::Block());
}
