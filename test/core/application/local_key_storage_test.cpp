/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/impl/local_key_storage.hpp"

#include <gtest/gtest.h>

#include <libp2p/crypto/crypto_provider/crypto_provider_impl.hpp>
#include <libp2p/crypto/ed25519_provider/ed25519_provider_impl.hpp>
#include <libp2p/crypto/key_validator/key_validator_impl.hpp>
#include <libp2p/crypto/random_generator/boost_generator.hpp>
#include <testutil/outcome.hpp>

#include "application/impl/key_storage_error.hpp"

using kagome::application::KeyStorageError;
using kagome::application::LocalKeyStorage;
using libp2p::crypto::CryptoProviderImpl;
using libp2p::crypto::ed25519::Ed25519ProviderImpl;
using libp2p::crypto::random::BoostRandomGenerator;
using libp2p::crypto::validator::KeyValidatorImpl;

class LocalKeyStorageTest : public testing::Test {
 public:
  void SetUp() {
    auto random_generator = std::make_shared<BoostRandomGenerator>();
    auto ed25519_provider = std::make_shared<Ed25519ProviderImpl>();

    crypto_provider = std::make_shared<CryptoProviderImpl>(
        std::move(random_generator), std::move(ed25519_provider));
    validator = std::make_shared<KeyValidatorImpl>(crypto_provider);
    auto path = boost::filesystem::path(__FILE__).parent_path().string();
    config.ed25519_keypair_location = path + "/ed25519key.pem";
    config.sr25519_keypair_location = path + "/sr25519key.txt";
    config.p2p_keypair_location = path + "/ed25519key.pem";
    config.p2p_keypair_type = libp2p::crypto::Key::Type::Ed25519;
  }

  LocalKeyStorage::Config config;
  std::shared_ptr<CryptoProviderImpl> crypto_provider;
  std::shared_ptr<KeyValidatorImpl> validator;
};

TEST_F(LocalKeyStorageTest, CreateWithEd25519) {
  auto s = LocalKeyStorage::create(config, crypto_provider, validator);
  EXPECT_OUTCOME_TRUE_1(s);
}

TEST_F(LocalKeyStorageTest, FileNotFound) {
  config.p2p_keypair_location = "aaa";
  auto s = LocalKeyStorage::create(config, crypto_provider, validator);
  EXPECT_OUTCOME_FALSE(e, s);
  ASSERT_EQ(e, KeyStorageError::FILE_READ_ERROR);
}

TEST_F(LocalKeyStorageTest, UnsupportedKeyType) {
  config.p2p_keypair_type = libp2p::crypto::Key::Type::UNSPECIFIED;
  auto s = LocalKeyStorage::create(config, crypto_provider, validator);
  EXPECT_OUTCOME_FALSE(e, s);
  ASSERT_EQ(e, KeyStorageError::UNSUPPORTED_KEY_TYPE);
}

TEST_F(LocalKeyStorageTest, InvalidKey) {
  config.ed25519_keypair_location =
      boost::filesystem::path(__FILE__).parent_path().string()
      + "/wrong_ed25519key.pem";
  auto s = LocalKeyStorage::create(config, crypto_provider, validator);
  EXPECT_OUTCOME_FALSE(e, s);
  ASSERT_EQ(e, KeyStorageError::PRIVATE_KEY_READ_ERROR);
}
