/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <boost/algorithm/string/join.hpp>
#include "common/buffer.hpp"
#include "crypto/bip39/entropy_accumulator.hpp"
#include "crypto/bip39/impl/bip39_provider_impl.hpp"
#include "crypto/bip39/mnemonic.hpp"
#include "crypto/pbkdf2/impl/pbkdf2_provider_impl.hpp"
#include "testutil/outcome.hpp"
#include "common/blob.hpp"

using namespace kagome;
using namespace crypto;
using namespace bip39;

struct TestItem {
  std::string mnemonic;
  std::string entropy;
  std::string seed;
};

struct Bip39EntropyTest : public ::testing::Test {
  void SetUp() override {
    auto pbkdf2_provider = std::make_shared<Pbkdf2ProviderImpl>();
    bip39_provider = std::make_shared<Bip39ProviderImpl>(pbkdf2_provider);
  }

  std::shared_ptr<Bip39Provider> bip39_provider;
};

TEST_F(Bip39EntropyTest, DecodeSuccess) {
  const TestItem item = {
      "legal winner thank year wave sausage worth useful legal winner "
      "thank yellow",
      "7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f",
      "4313249608fe8ac10fd5886c92c4579007272cb77c21551ee5b8d60b780416850"
      "f1e"
      "26c1f4b8d88ece681cb058ab66d6182bc2ce5a03181f7b74c27576b5c8bf",
  };

  EXPECT_OUTCOME_TRUE(mnemonic, Mnemonic::parse(item.mnemonic));
  auto joined_words = boost::algorithm::join(mnemonic.words, " ");
  ASSERT_EQ(joined_words, item.mnemonic);

  EXPECT_OUTCOME_TRUE(entropy,
                      bip39_provider->calculateEntropy(mnemonic.words));
  std::cout << common::Buffer(entropy).toHex() << std::endl;

  std::cout << kagome::common::Buffer(entropy).toHex() << std::endl;

  EXPECT_OUTCOME_TRUE(seed, bip39_provider->makeSeed(entropy, "Substrate"));
  ASSERT_EQ(seed.toHex(), item.seed);
}
