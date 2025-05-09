/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <boost/algorithm/string/join.hpp>
#include <qtils/test/outcome.hpp>

#include <mock/libp2p/crypto/random_generator_mock.hpp>
#include "common/blob.hpp"
#include "common/buffer.hpp"
#include "crypto/bip39/impl/bip39_provider_impl.hpp"
#include "crypto/bip39/mnemonic.hpp"
#include "crypto/hasher/hasher_impl.hpp"
#include "crypto/pbkdf2/impl/pbkdf2_provider_impl.hpp"
#include "testutil/prepare_loggers.hpp"

using namespace kagome;
using namespace crypto;
using namespace bip39;

struct Bip39EntropyTest : public ::testing::Test {
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {
    auto pbkdf2_provider = std::make_shared<Pbkdf2ProviderImpl>();
    auto hasher = std::make_shared<HasherImpl>();
    bip39_provider = std::make_shared<Bip39ProviderImpl>(
        pbkdf2_provider,
        std::make_shared<libp2p::crypto::random::CSPRNGMock>(),
        hasher);
    phrase =
        "legal winner thank year wave sausage worth useful legal winner "
        "thank yellow";
    entropy_hex = "7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f";
    seed_hex =
        "4313249608fe8ac10fd5886c92c4579007272cb77c21551ee5b8d60b780416850"
        "f1e26c1f4b8d88ece681cb058ab66d6182bc2ce5a03181f7b74c27576b5c8bf";
  }

  std::string_view phrase;
  std::string_view entropy_hex;
  std::string_view seed_hex;

  std::shared_ptr<Bip39Provider> bip39_provider;
};

/**
 * @given valid mnemonic, entropy and seed values
 * @when entropy is calculated by mnemonic
 * @and seed is calculated by entropy
 * @then entropy and seed come up with predefined values
 */
TEST_F(Bip39EntropyTest, DecodeSuccess) {
  ASSERT_OUTCOME_SUCCESS(mnemonic, Mnemonic::parse(phrase));
  auto joined_words = boost::algorithm::join(*mnemonic.words(), " ");
  ASSERT_EQ(joined_words, phrase);

  ASSERT_OUTCOME_SUCCESS(entropy,
                         bip39_provider->calculateEntropy(*mnemonic.words()));
  ASSERT_EQ(common::Buffer(entropy).toHex(), entropy_hex);

  ASSERT_OUTCOME_SUCCESS(seed, bip39_provider->makeSeed(entropy, "Substrate"));
  ASSERT_EQ(seed, common::unhex(seed_hex).value());
}
