/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/sr25519/sr25519_provider_impl.hpp"

#include <gtest/gtest.h>
#include <gsl/span>
#include "crypto/random_generator/boost_generator.hpp"
#include "testutil/outcome.hpp"

using kagome::crypto::BoostRandomGenerator;
using kagome::crypto::CSPRNG;
using kagome::crypto::Sr25519Provider;
using kagome::crypto::Sr25519ProviderImpl;
using kagome::crypto::Sr25519PublicKey;
using kagome::crypto::Sr25519SecretKey;
using kagome::crypto::Sr25519Seed;

struct Sr25519ProviderTest : public ::testing::Test {
  void SetUp() override {
    random_generator = std::make_shared<BoostRandomGenerator>();
    sr25519_provider = std::make_shared<Sr25519ProviderImpl>(random_generator);

    std::string_view m = "i am a message";
    message.clear();
    message.reserve(m.length());
    for (auto c : m) {
      message.push_back(c);
    }

    hex_seed =
        "31102468cbd502d177793fa523685b248f6bd083d67f76671e0b86d7fa20c030";
    hex_sk =
        "e5aff1a7d9694f2c0505f41ca68d51093d4f9f897aaa3ec4116b80393690010bbb5ee"
        "1ea15ca731e60cd92b0765cf00675bb7eeabc04e531629988cd90e53ad6";
    hex_vk = "6221d74b4c2168d0f73f97589900d2c6bdcdf3a8d54c3c92adc9e7650fbff251";
  }

  std::string_view hex_seed;
  std::string_view hex_sk;
  std::string_view hex_vk;

  gsl::span<uint8_t> message_span;
  std::vector<uint8_t> message;
  std::shared_ptr<CSPRNG> random_generator;
  std::shared_ptr<Sr25519Provider> sr25519_provider;
};

/**
 * @given sr25519 provider instance configured with boost random generator
 * @when generate 2 keypairs, repeat it 10 times
 * @then each time keys are different
 */
TEST_F(Sr25519ProviderTest, GenerateKeysNotEqual) {
  for (auto i = 0; i < 10; ++i) {
    auto kp1 = sr25519_provider->generateKeypair();
    auto kp2 = sr25519_provider->generateKeypair();
    ASSERT_NE(kp1.public_key, kp2.public_key);
    ASSERT_NE(kp1.secret_key, kp2.secret_key);
  }
}

/**
 * @given sr25519 provider instance configured with boost random generator
 * @and a predefined message
 * @when generate a keypair @and sign message
 * @and verify signed message with generated public key
 * @then verification succeeds
 */
TEST_F(Sr25519ProviderTest, SignVerifySuccess) {
  auto kp = sr25519_provider->generateKeypair();
  EXPECT_OUTCOME_TRUE(signature, sr25519_provider->sign(kp, message_span));
  EXPECT_OUTCOME_TRUE(
      res, sr25519_provider->verify(signature, message_span, kp.public_key));
  ASSERT_EQ(res, true);
}

/**
 * Don't try to sign a message using invalid key pair, this may lead to
 * program termination
 *
 * @given sr25519 provider instance configured with boost random generator
 * @and a predefined message
 * @when generate a keypair @and make public key invalid @and sign message
 * @then sign fails
 */
TEST_F(Sr25519ProviderTest, DISABLED_SignWithInvalidKeyFails) {
  auto kp = sr25519_provider->generateKeypair();
  kp.public_key.fill(1);
  EXPECT_OUTCOME_FALSE_1(sr25519_provider->sign(kp, message_span));
}

/**
 * @given sr25519 provider instance configured with boost random generator
 * @and and a predefined message
 * @when generate keypair @and sign message @and take another public key
 * @and verify signed message
 * @then verification succeeds, but verification result is false
 */
TEST_F(Sr25519ProviderTest, VerifyWrongKeyFail) {
  auto kp = sr25519_provider->generateKeypair();
  EXPECT_OUTCOME_TRUE(signature, sr25519_provider->sign(kp, message_span));
  // generate another valid key pair and take public one
  auto kp1 = sr25519_provider->generateKeypair();
  EXPECT_OUTCOME_TRUE(
      ver_res,
      sr25519_provider->verify(signature, message_span, kp1.public_key));

  ASSERT_FALSE(ver_res);
}

/**
 * Don't try to verify a message and signature against an invalid key, this may
 * lead to program termination
 *
 * @given sr25519 provider instance configured with boost random generator
 * @and and a predefined message
 * @when generate keypair @and sign message
 * @and generate another keypair and take public part for verification
 * @and verify signed message
 * @then verification fails
 */
TEST_F(Sr25519ProviderTest, DISABLED_VerifyInvalidKeyFail) {
  auto kp = sr25519_provider->generateKeypair();
  EXPECT_OUTCOME_TRUE(signature, sr25519_provider->sign(kp, message_span));
  // make public key invalid
  kp.public_key.fill(1);
  EXPECT_OUTCOME_FALSE_1(
      sr25519_provider->verify(signature, message_span, kp.public_key));
}

/**
 * @given seed value
 * @when generate key pair by seed
 * @then verifying and secret keys come up with predefined values
 */
TEST_F(Sr25519ProviderTest, GenerateBySeedSuccess) {
  EXPECT_OUTCOME_TRUE(seed, Sr25519Seed::fromHex(hex_seed));
  EXPECT_OUTCOME_TRUE(public_key, Sr25519PublicKey::fromHex(hex_vk));

  // private key is the same as seed
  EXPECT_OUTCOME_TRUE(secret_key, Sr25519SecretKey::fromHex(hex_sk));

  auto &&kp = sr25519_provider->generateKeypair(seed);

  ASSERT_EQ(kp.secret_key, secret_key);
  ASSERT_EQ(kp.public_key, public_key);
}
