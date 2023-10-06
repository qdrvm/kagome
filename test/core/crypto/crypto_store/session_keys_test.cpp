/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "crypto/crypto_store/session_keys.hpp"
#include "mock/core/application/app_configuration_mock.hpp"
#include "mock/core/crypto/crypto_store_mock.hpp"
#include "testutil/prepare_loggers.hpp"

using kagome::application::AppConfigurationMock;
using namespace kagome;
using namespace crypto;

using testing::_;
using testing::Return;
using Ed25519Keys = std::vector<Ed25519PublicKey>;
using Sr25519Keys = std::vector<Sr25519PublicKey>;

struct SessionKeysTest : public ::testing::Test {
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {
    store = std::make_shared<CryptoStoreMock>();
    role.flags.authority = 1;
    EXPECT_CALL(*config, roles()).WillOnce(Return(role));
    session_keys = std::make_shared<SessionKeysImpl>(store, *config);
  }

  std::shared_ptr<AppConfigurationMock> config =
      std::make_shared<AppConfigurationMock>();
  network::Roles role;
  std::shared_ptr<CryptoStoreMock> store;
  std::shared_ptr<SessionKeysImpl> session_keys;
};

/**
 * @given an empty session keys store
 * @when having inserted keys into it
 * @then session keys are initialized with inserted keys of the corresponding
 * types
 */
TEST_F(SessionKeysTest, SessionKeys) {
  outcome::result<Ed25519Keys> ed_keys_empty = Ed25519Keys{};
  outcome::result<Sr25519Keys> sr_keys_empty = Sr25519Keys{};
  EXPECT_CALL(*store, getSr25519PublicKeys(KeyType(KeyTypes::BABE)))
      .Times(1)
      .WillOnce(Return(sr_keys_empty));
  EXPECT_CALL(*store, getEd25519PublicKeys(KeyType(KeyTypes::GRANDPA)))
      .Times(1)
      .WillOnce(Return(ed_keys_empty));
  ASSERT_FALSE(session_keys->getBabeKeyPair({}));
  ASSERT_FALSE(session_keys->getGranKeyPair({}));

  auto ed_key = Ed25519PublicKey::fromHex(
                    "3e765f2bde3daadd443097b3145abf1f71f99f0aa946"
                    "960990fe02aa26b7fc72")
                    .value();

  auto sr_key = Sr25519PublicKey::fromHex(
                    "56a03c8afc0e7a3a8b1d53bcc875ba5b6364754f9045"
                    "16009b57ef3adf96f61f")
                    .value();

  outcome::result<Ed25519Keys> ed_keys = Ed25519Keys{ed_key};
  outcome::result<Sr25519Keys> sr_keys = Sr25519Keys{sr_key};

  EXPECT_CALL(*store, getSr25519PublicKeys(KeyType(KeyTypes::BABE)))
      .Times(1)
      .WillOnce(Return(sr_keys));
  EXPECT_CALL(*store, getEd25519PublicKeys(KeyType(KeyTypes::GRANDPA)))
      .Times(1)
      .WillOnce(Return(ed_keys));

  auto ed_priv = Ed25519PrivateKey::fromHex(
                     "a4681403ba5b6a3f3bd0b0604ce439a78244c7d43b1"
                     "27ec35cd8325602dd47fd")
                     .value();

  auto sr_priv =
      Sr25519SecretKey::fromHex(
          "ec96cb0816b67b045baae21841952a61ecb0612a109293e10c5453b950659c0a8b"
          "35b6d6196f33169334e36a05d624d9996d07243f9f71e638e3bc29a5330ec9")
          .value();

  outcome::result<Ed25519Keypair> ed_pair = Ed25519Keypair{ed_priv, ed_key};
  outcome::result<Sr25519Keypair> sr_pair = Sr25519Keypair{sr_priv, sr_key};

  EXPECT_CALL(*store, findSr25519Keypair(KeyType(KeyTypes::BABE), _))
      .Times(1)
      .WillOnce(Return(sr_pair));
  EXPECT_CALL(*store, findEd25519Keypair(KeyType(KeyTypes::GRANDPA), _))
      .Times(1)
      .WillOnce(Return(ed_pair));
  ASSERT_TRUE(session_keys->getBabeKeyPair({{{sr_key}, {}}}));
  ASSERT_TRUE(session_keys->getGranKeyPair({{}, {{{ed_key}, {}}}}));

  // no additional calls to store, reads from cache
  ASSERT_TRUE(session_keys->getBabeKeyPair({{{sr_key}, {}}}));
  ASSERT_TRUE(session_keys->getGranKeyPair({{}, {{{ed_key}, {}}}}));
}
