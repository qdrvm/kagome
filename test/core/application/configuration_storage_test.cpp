/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <boost/filesystem/path.hpp>
#include "application/impl/configuration_storage_impl.hpp"
#include "testutil/outcome.hpp"

using kagome::application::ConfigurationStorageImpl;
using kagome::blockchain::GenesisRawConfig;
using kagome::common::Buffer;
using kagome::crypto::SR25519PublicKey;
using libp2p::multi::Multiaddress;
using libp2p::peer::PeerId;
using libp2p::peer::PeerInfo;

class ConfigurationStorageTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Fill configs with the same values as in the genesis config from file
    // under the `path_`
    PeerInfo peer_info{
        .id =
            PeerId::fromBase58("QmWfTgC2DEt9FhPoccnh5vT5xM5wqWy37EnAPZFQgqheZ6")
                .value(),
        .addresses = {Multiaddress::create(
                          "/ip4/127.0.0.1/tcp/30363/ipfs/"
                          "QmWfTgC2DEt9FhPoccnh5vT5xM5wqWy37EnAPZFQgqheZ6")
                          .value()}};
    expected_boot_nodes_.push_back(peer_info);

    std::vector<std::pair<std::string, std::string>> genesis_raw_configs = {
        std::make_pair("01", "aa"), std::make_pair("02", "bb")};
    for (auto &[key_hex, val_hex] : genesis_raw_configs) {
      expected_genesis_config_.emplace_back(Buffer::fromHex(key_hex).value(),
                                            Buffer::fromHex(val_hex).value());
    }

    std::vector<std::string> srkeys{
        "7ad7336e38e0ddd6635fb4cc88e65ddc0c9fdaa65ecf3d131c9db9c391834450",
        "0c9fdaa65ecf3d131c9db9c3918344507ad7336e38e0ddd6635fb4cc88e65ddc"};
    for (auto &srkey_hex : srkeys) {
      expected_session_keys_.push_back(
          SR25519PublicKey::fromHex(srkey_hex).value());
    }
  }

  std::string path_ =
      boost::filesystem::path(__FILE__).parent_path().string()
      + "/genesis.json";  // < Path to file containing the following configs:
  std::vector<libp2p::peer::PeerInfo> expected_boot_nodes_;
  GenesisRawConfig expected_genesis_config_;
  std::vector<SR25519PublicKey> expected_session_keys_;
};

/**
 * @given path to valid config file (genesis.json)
 * @when creating a configuration storage with the given config
 * @then the content of the storage matches expected content
 */
TEST_F(ConfigurationStorageTest, MatchesConfig) {
  // given provided in set up
  // when
  EXPECT_OUTCOME_TRUE(config_storage, ConfigurationStorageImpl::create(path_));

  // then
  ASSERT_EQ(config_storage->getGenesis(), expected_genesis_config_);
  ASSERT_EQ(config_storage->getBootNodes(), expected_boot_nodes_);
  ASSERT_EQ(config_storage->getSessionKeys(), expected_session_keys_);
}
