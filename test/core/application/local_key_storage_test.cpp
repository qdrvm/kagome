/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/impl/local_key_storage.hpp"

#include <gtest/gtest.h>

#include <testutil/outcome.hpp>
#include "application/impl/config_reader/error.hpp"

using kagome::application::ConfigReaderError;
using kagome::application::LocalKeyStorage;
using kagome::crypto::ED25519Keypair;
using kagome::crypto::ED25519PrivateKey;
using kagome::crypto::ED25519PublicKey;
using kagome::crypto::SR25519Keypair;
using kagome::crypto::SR25519PublicKey;
using kagome::crypto::SR25519SecretKey;

class LocalKeyStorageTest : public testing::Test {
 public:
  void SetUp() override {
    // setting keystore_path_
    auto path = boost::filesystem::path(__FILE__).parent_path().string();
    keystore_path_ = path + "/keystore.json";

    prepareSR25519Keys();
    prepareED25519Keys();
    prepareP2PKeys();
  }

  // initialise expected sr25519 keys
  void prepareSR25519Keys() {
    auto expected_srpubkey =
        SR25519PublicKey::fromHex(
            "7ad7336e38e0ddd6635fb4cc88e65ddc0c9fdaa65ecf3d131c9db9c391834450")
            .value();
    auto expected_srprivkey =
        SR25519SecretKey::fromHex(
            "e968852cf33994c02e4c81377acb9ce328fc25cb25dc6a7323c742b0e94d830dae"
            "97e44e79872c67fd8a4c29ce6a386fec71e46bc4b2f2f7e9887d770af3ed15")
            .value();
    expected_sr_keypair_ = SR25519Keypair{.public_key = expected_srpubkey,
                                          .secret_key = expected_srprivkey};
  }

  // initialise expected ed25519 keys
  void prepareED25519Keys() {
    auto expected_edpubkey =
        ED25519PublicKey::fromHex(
            "d046dde66d247e98e6c95366c05b6137ffeb61e9ee8541200569e70ac7632a46")
            .value();
    auto expected_edprivkey =
        ED25519PrivateKey::fromHex(
            "62f4174222f712edc938fa7fbdd06928967e91354e22f6c2aa097451aa5b03e4")
            .value();
    expected_ed_keypair_ = ED25519Keypair{.public_key = expected_edpubkey,
                                          .private_key = expected_edprivkey};
    expected_ed_keypair_.private_key = expected_edprivkey;
    expected_ed_keypair_.public_key = expected_edpubkey;
  }

  // initialise expected p2p keys
  void prepareP2PKeys() {
    libp2p::crypto::PublicKey p2p_pubkey;
    p2p_pubkey.data = kagome::common::unhex(
                          "3fb8ffa2f039a097951fcfcfd97e4257e77f74"
                          "a4937b9c208ed1f04e432fd7dd")
                          .value();
    p2p_pubkey.type = libp2p::crypto::Key::Type::Ed25519;

    libp2p::crypto::PrivateKey p2p_privkey;
    p2p_privkey.data =
        kagome::common::unhex(
            "7b84a15c9588536d17f34fc892342bab28cbb59ffa3438dc62e6b87131aa212d")
            .value();
    p2p_privkey.type = libp2p::crypto::Key::Type::Ed25519;

    expected_p2p_keypair_.privateKey = p2p_privkey;
    expected_p2p_keypair_.publicKey = p2p_pubkey;
  }

  SR25519Keypair expected_sr_keypair_;
  ED25519Keypair expected_ed_keypair_;
  libp2p::crypto::KeyPair expected_p2p_keypair_;

  std::string keystore_path_;
};

/**
 * @given keystore containing expected keys
 * @when keys are retrieved from the store
 * @then retrieved keys are the same with expected keys
 */
TEST_F(LocalKeyStorageTest, ValidKeyStore) {
  EXPECT_OUTCOME_TRUE(keystore, LocalKeyStorage::create(keystore_path_));
  ASSERT_EQ(keystore->getLocalSr25519Keypair(), expected_sr_keypair_);
  ASSERT_EQ(keystore->getLocalEd25519Keypair(), expected_ed_keypair_);
  ASSERT_EQ(keystore->getP2PKeypair(), expected_p2p_keypair_);
}

/**
 * @given invalid path to keystore
 * @when LocalKeyStorage is created from invalid path
 * @then PARSER_ERROR is returned
 */
TEST_F(LocalKeyStorageTest, FileNotFound) {
  keystore_path_ = "aaa";  // invalid path
  auto s = LocalKeyStorage::create(keystore_path_);
  EXPECT_OUTCOME_FALSE(e, s);
  ASSERT_EQ(e, ConfigReaderError::PARSER_ERROR);
}
