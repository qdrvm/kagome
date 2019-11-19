/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/impl/config_reader/json_configuration_reader.hpp"

#include <gtest/gtest.h>

#include <boost/filesystem.hpp>
#include <libp2p/multi/multibase_codec/codecs/base58.hpp>

#include "application/impl/config_reader/error.hpp"
#include "primitives/block.hpp"
#include "scale/scale.hpp"
#include "testutil/outcome.hpp"

using kagome::application::ConfigReaderError;
using kagome::application::JsonConfigurationReader;
using kagome::application::KagomeConfig;
using libp2p::multi::detail::encodeBase58;
using libp2p::peer::PeerInfo;
using libp2p::peer::PeerId;
using libp2p::multi::Multiaddress;
using kagome::crypto::SR25519PublicKey;
using kagome::crypto::ED25519PublicKey;
using kagome::common::unhex;

const KagomeConfig& getExampleConfig() {
  static bool init = true;
  static KagomeConfig c;
  if (init) {
    c.genesis.header.number = 42;
    c.api_ports.extrinsic_api_port = 4224;
    c.peers_info = {
        PeerInfo{
            PeerId::fromBase58("1AWR4A2YXCzotpPjJshv1QUwSTExoYWiwr33C4briAGpCY")
                .value(),
            {Multiaddress::create("/ip4/127.0.0.1/udp/1234").value(),
             Multiaddress::create("/ipfs/mypeer").value()}},
        PeerInfo{
            PeerId::fromBase58("1AWUyTAqzDb7C3XpZP9DLKmpDDV81kBndfbSrifEkm29XF")
                .value(),
            {Multiaddress::create("/ip4/127.0.0.1/tcp/1020").value(),
             Multiaddress::create("/ipfs/mypeer").value()}}};
    c.session_keys = {
        SR25519PublicKey::fromHex(
            "0101010101010101010101010101010101010101010101010101010101010101")
            .value(),
        SR25519PublicKey::fromHex(
            "0202020202020202020202020202020202020202020202020202020202020202")
            .value()};
    c.authorities = {
        ED25519PublicKey::fromHex(
            "0101010101010101010101010101010101010101010101010101010101010101")
            .value(),
        ED25519PublicKey::fromHex(
            "0202020202020202020202020202020202020202020202020202020202020202")
            .value()};
    init = false;
  }
  return c;
}

std::stringstream readJSONConfig() {
  std::ifstream f(boost::filesystem::path(__FILE__).parent_path().string()
                  + "/example_config.json");
  if (!f) {
    throw std::runtime_error("config file not found");
  }
  std::stringstream ss;
  while (f) {
    std::string s;
    f >> s;
    ss << s;
  }
  return ss;
}

/**
 * @given a json file with configuration
 * @when initialising configuration storage from this file
 * @then the content of the storage matches the content of the file
 */
TEST(JsonConfigReader, LoadConfig) {
  auto& c = getExampleConfig();
  auto ss = readJSONConfig();
  EXPECT_OUTCOME_TRUE(config, JsonConfigurationReader::initConfig(ss));
  ASSERT_EQ(config.genesis, c.genesis);
  ASSERT_EQ(config.api_ports.extrinsic_api_port,
            c.api_ports.extrinsic_api_port);
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
  auto ss = readJSONConfig();
  // read INIT_NUMBER back from JSON
  EXPECT_OUTCOME_TRUE_1(JsonConfigurationReader::updateConfig(config, ss));
  ASSERT_EQ(config.genesis.header.number, 42);
  ASSERT_EQ(config.api_ports.extrinsic_api_port, 4224);
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
