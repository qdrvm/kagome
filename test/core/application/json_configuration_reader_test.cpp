/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/impl/config_reader/json_configuration_reader.hpp"

#include <gtest/gtest.h>

#include <libp2p/multi/multibase_codec/codecs/base58.hpp>
#include "application/impl/config_reader/error.hpp"
#include "primitives/block.hpp"
#include "scale/scale.hpp"
#include "testutil/outcome.hpp"

using kagome::application::ConfigReaderError;
using kagome::application::JsonConfigurationReader;
using kagome::application::KagomeConfig;
using libp2p::multi::detail::encodeBase58;

static std::stringstream createJSONConfig(const KagomeConfig &conf) {
  std::stringstream config_data;
  using libp2p::peer::PeerId;
  using namespace libp2p::crypto;
  auto example_pubkey1 = ProtobufKey{std::vector<uint8_t>(32, 1)};
  auto example_pubkey2 = ProtobufKey{std::vector<uint8_t>(32, 2)};
  config_data << "{\n";
  config_data << "\t\"genesis\":\""
              << kagome::common::hex_lower(
                     kagome::scale::encode(conf.genesis).value())
              << "\",\n";
  config_data << "\t\"peer_ids\":[\""
              << PeerId::fromPublicKey(example_pubkey1).value().toBase58() << "\", \""
              << PeerId::fromPublicKey(example_pubkey2).value().toBase58() << "\"],\n";
  config_data << "\t\"authorities\":[\""
              << kagome::common::hex_lower(example_pubkey1.key)
              << "\", \""
              << kagome::common::hex_lower(example_pubkey2.key)
              << "\"],\n";
  config_data << "\t\"session_keys\":[\""
              << kagome::common::hex_lower(example_pubkey1.key)
              << "\", \""
              << kagome::common::hex_lower(example_pubkey2.key)
              << "\"]\n";
  config_data << "}\n";
  return config_data;
}

/**
 * @given a json file with configuration
 * @when initialising configuration storage from this file
 * @then the content of the storage matches the content of the file
 */
TEST(JsonConfigReader, LoadConfig) {
  KagomeConfig c;
  c.genesis = kagome::primitives::Block();
  c.genesis.header.number = 42;
  auto ss = createJSONConfig(c);
  EXPECT_OUTCOME_TRUE(config, JsonConfigurationReader::initConfig(ss));
  ASSERT_EQ(config.genesis, c.genesis);
}

/**
 * @given a json file with configuration
 * @when updating configuration storage from this file
 * @then the content of the storage matches the content of the file
 */
TEST(JsonConfigReader, UpdateConfig) {
  constexpr int INIT_NUMBER = 34;
  constexpr int NEW_NUMBER = 42;
  KagomeConfig config;
  config.genesis = kagome::primitives::Block();
  config.genesis.header.number = INIT_NUMBER;
  auto ss = createJSONConfig(config);
  // overwrite number in the config, but it is unchanged in the json
  config.genesis.header.number = NEW_NUMBER;
  // read INIT_NUMBER back from JSON
  EXPECT_OUTCOME_TRUE_1(JsonConfigurationReader::updateConfig(config, ss));
  ASSERT_EQ(config.genesis.header.number, INIT_NUMBER);
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
