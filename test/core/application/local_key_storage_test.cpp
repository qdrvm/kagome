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
#include "application/impl/config_reader/error.hpp"

using kagome::application::ConfigReaderError;
using kagome::application::LocalKeyStorage;
using libp2p::crypto::random::BoostRandomGenerator;
using libp2p::crypto::validator::KeyValidatorImpl;

class LocalKeyStorageTest : public testing::Test {
 public:
  void SetUp() override {
    auto path = boost::filesystem::path(__FILE__).parent_path().string();
    config.keystore_path = path + "/keystore.json";
  }

  LocalKeyStorage::Config config;
  std::shared_ptr<KeyValidatorImpl> validator;
};

TEST_F(LocalKeyStorageTest, CreateWithEd25519) {
  auto s = LocalKeyStorage::create(config);
  EXPECT_OUTCOME_TRUE_1(s);
}

TEST_F(LocalKeyStorageTest, FileNotFound) {
  config.keystore_path = "aaa";
  auto s = LocalKeyStorage::create(config);
  EXPECT_OUTCOME_FALSE(e, s);
  ASSERT_EQ(e, ConfigReaderError::PARSER_ERROR);
}
