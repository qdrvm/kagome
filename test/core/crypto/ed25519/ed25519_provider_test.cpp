/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <gsl/span>

#include "crypto/ed25519/ed25519_provider_impl.hpp"
#include "crypto/random_generator/boost_generator.hpp"
#include "testutil/outcome.hpp"
#include "testutil/prepare_loggers.hpp"

using kagome::crypto::BoostRandomGenerator;
using kagome::crypto::Ed25519PrivateKey;
using kagome::crypto::Ed25519Provider;
using kagome::crypto::Ed25519ProviderImpl;
using kagome::crypto::Ed25519PublicKey;
using kagome::crypto::Ed25519Seed;

struct Ed25519ProviderTest : public ::testing::Test {
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {
    auto random_generator = std::make_shared<BoostRandomGenerator>();
    ed25519_provider = std::make_shared<Ed25519ProviderImpl>(random_generator);

    std::string_view m = "i am a message";
    message = std::vector<uint8_t>(m.begin(), m.end());
    message_span = message;

    hex_seed =
        "ccb4ec79974db3dae0d4dff7e0963db6b798684356dc517ff5c2e61f3b641569";
    hex_public_key =
        "767a2f677a8c704d66e2abbb181d8984adae7ac8ecac9e30709ad496244ab497";
  }

  std::string_view hex_seed;
  std::string_view hex_public_key;

  gsl::span<uint8_t> message_span;
  std::vector<uint8_t> message;
  std::shared_ptr<Ed25519Provider> ed25519_provider;
};

/**
 * @given ed25519 provider instance configured with boost random generator
 * @when generate 2 keypairs, repeat it 10 times
 * @then each time keys are different
 */
TEST_F(Ed25519ProviderTest, GenerateKeysNotEqual) {
  for (auto i = 0; i < 10; ++i) {
    auto kp1 = ed25519_provider->generateKeypair();
    auto kp2 = ed25519_provider->generateKeypair();
    ASSERT_NE(kp1.public_key, kp2.public_key);
    ASSERT_NE(kp1.secret_key, kp2.secret_key);
  }
}

/**
 * @given ed25519 provider instance configured with boost random generator
 * @and a predefined message
 * @when generate a keypair @and sign message
 * @and verify signed message with generated public key
 * @then verification succeeds
 */
TEST_F(Ed25519ProviderTest, SignVerifySuccess) {
  auto kp = ed25519_provider->generateKeypair();
  EXPECT_OUTCOME_TRUE(signature, ed25519_provider->sign(kp, message_span));
  EXPECT_OUTCOME_TRUE(
      res, ed25519_provider->verify(signature, message_span, kp.public_key));
  ASSERT_EQ(res, true);
}

/**
 * Don't try to sign a message using invalid key pair, this may lead to
 * program termination
 *
 * @given ed25519 provider instance configured with boost random generator
 * @and a predefined message
 * @when generate a keypair @and make public key invalid @and sign message
 * @then sign fails
 */
TEST_F(Ed25519ProviderTest, SignWithInvalidKeyFails) {
  auto kp = ed25519_provider->generateKeypair();
  kp.public_key.fill(1);
  EXPECT_OUTCOME_FALSE_1(ed25519_provider->sign(kp, message_span));
}

/**
 * @given ed25519 provider instance configured with boost random generator
 * @and and a predefined message
 * @when generate keypair @and sign message @and take another public key
 * @and verify signed message
 * @then verification succeeds, but verification result is false
 */
TEST_F(Ed25519ProviderTest, VerifyWrongKeyFail) {
  auto kp = ed25519_provider->generateKeypair();
  EXPECT_OUTCOME_TRUE(signature, ed25519_provider->sign(kp, message_span));
  // generate another valid key pair and take public one
  auto kp1 = ed25519_provider->generateKeypair();
  EXPECT_OUTCOME_TRUE(
      ver_res,
      ed25519_provider->verify(signature, message_span, kp1.public_key));

  ASSERT_FALSE(ver_res);
}

/**
 * Don't try to verify a message and signature against an invalid key, this may
 * lead to program termination
 *
 * @given ed25519 provider instance configured with boost random generator
 * @and and a predefined message
 * @when generate keypair @and sign message
 * @and generate another keypair and take public part for verification
 * @and verify signed message
 * @then verification fails
 */
TEST_F(Ed25519ProviderTest, DISABLED_VerifyInvalidKeyFail) {
  auto kp = ed25519_provider->generateKeypair();
  EXPECT_OUTCOME_TRUE(signature, ed25519_provider->sign(kp, message_span));
  // make public key invalid
  kp.public_key.fill(1);
  EXPECT_OUTCOME_FALSE_1(
      ed25519_provider->verify(signature, message_span, kp.public_key));
}

/**
 * @given seed value
 * @when generate key pair by seed
 * @then public and private keys come up with predefined values
 */
TEST_F(Ed25519ProviderTest, GenerateBySeedSuccess) {
  EXPECT_OUTCOME_TRUE(seed, Ed25519Seed::fromHex(hex_seed));
  EXPECT_OUTCOME_TRUE(public_key, Ed25519PublicKey::fromHex(hex_public_key));

  // private key is the same as seed
  EXPECT_OUTCOME_TRUE(private_key, Ed25519PrivateKey::fromHex(hex_seed));

  auto kp = ed25519_provider->generateKeypair(seed);

  ASSERT_EQ(kp.secret_key, private_key);
  ASSERT_EQ(kp.public_key, public_key);
}
