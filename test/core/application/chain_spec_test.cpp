/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "application/impl/chain_spec_impl.hpp"
#include "filesystem/common.hpp"
#include "testutil/outcome.hpp"
#include "testutil/prepare_loggers.hpp"

using kagome::application::ChainSpecImpl;
using kagome::application::GenesisRawData;
using kagome::common::Buffer;
using kagome::crypto::Sr25519PublicKey;
using libp2p::multi::Multiaddress;
using libp2p::peer::PeerId;
using libp2p::peer::PeerInfo;

class ConfigurationStorageTest : public ::testing::Test {
 protected:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {
    // Fill configs with the same values as in the genesis config from file
    // under the `path_`
    expected_boot_nodes_.push_back(
        Multiaddress::create("/ip4/127.0.0.1/tcp/30363/p2p/"
                             "QmWfTgC2DEt9FhPoccnh5vT5xM5wqWy37EnAPZFQgqheZ6")
            .value());

    std::vector<std::pair<std::string, std::string>> genesis_raw_configs = {
        std::make_pair("01", "aa"), std::make_pair("02", "bb")};
    for (auto &[key_hex, val_hex] : genesis_raw_configs) {
      expected_genesis_config_.emplace_back(Buffer::fromHex(key_hex).value(),
                                            Buffer::fromHex(val_hex).value());
    }
  }

  std::string path_ =
      kagome::filesystem::path(__FILE__).parent_path().string()
      + "/genesis.json";  // < Path to file containing the following configs:
  std::vector<libp2p::multi::Multiaddress> expected_boot_nodes_;
  GenesisRawData expected_genesis_config_;
};

/**
 * @given path to valid config file (genesis.json)
 * @when creating a configuration storage with the given config
 * @then the content of the storage matches expected content
 */
TEST_F(ConfigurationStorageTest, MatchesConfig) {
  // given provided in set up
  // when
  EXPECT_OUTCOME_TRUE(config_storage, ChainSpecImpl::loadFrom(path_));

  // then
  ASSERT_EQ(config_storage->getGenesisTopSection(), expected_genesis_config_);

  ASSERT_EQ(config_storage->bootNodes(), expected_boot_nodes_);
}
