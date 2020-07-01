/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/ed25519/ed25519_provider_impl.hpp"

#include <gtest/gtest.h>
#include <gsl/span>
#include "testutil/outcome.hpp"

using kagome::crypto::ED25519PrivateKey;
using kagome::crypto::ED25519Provider;
using kagome::crypto::ED25519ProviderImpl;
using kagome::crypto::ED25519PublicKey;
using kagome::crypto::ED25519Seed;

struct ED25519ProviderTest : public ::testing::Test {
  void SetUp() override {
    ed25519_provider = std::make_shared<ED25519ProviderImpl>();

    std::string_view m = "i am a message";
    message = std::vector<uint8_t>(m.begin(), m.end());

    hex_seed =
        "ccb4ec79974db3dae0d4dff7e0963db6b798684356dc517ff5c2e61f3b641569";
    hex_public_key =
        "767a2f677a8c704d66e2abbb181d8984adae7ac8ecac9e30709ad496244ab497";
  }

  std::string_view hex_seed;
  std::string_view hex_public_key;

  gsl::span<uint8_t> message_span;
  std::vector<uint8_t> message;
  std::shared_ptr<ED25519Provider> ed25519_provider;
};

/**
 * @given ed25519 provider instance configured with boost random generator
 * @when generate 2 keypairs, repeat it 10 times
 * @then each time keys are different
 */
TEST_F(ED25519ProviderTest, GenerateKeysNotEqual) {
  for (auto i = 0; i < 10; ++i) {
    EXPECT_OUTCOME_TRUE(kp1, ed25519_provider->generateKeypair());
    EXPECT_OUTCOME_TRUE(kp2, ed25519_provider->generateKeypair());
    ASSERT_NE(kp1.public_key, kp2.public_key);
    ASSERT_NE(kp1.private_key, kp2.private_key);
  }
}

/**
 * @given ed25519 provider instance configured with boost random generator
 * @and a predefined message
 * @when generate a keypair @and sign message
 * @and verify signed message with generated public key
 * @then verification succeeds
 */
TEST_F(ED25519ProviderTest, SignVerifySuccess) {
  EXPECT_OUTCOME_TRUE(kp, ed25519_provider->generateKeypair());
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
TEST_F(ED25519ProviderTest, DISABLED_SignWithInvalidKeyFails) {
  EXPECT_OUTCOME_TRUE(kp, ed25519_provider->generateKeypair());
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
TEST_F(ED25519ProviderTest, VerifyWrongKeyFail) {
  EXPECT_OUTCOME_TRUE(kp, ed25519_provider->generateKeypair());
  EXPECT_OUTCOME_TRUE(signature, ed25519_provider->sign(kp, message_span));
  // generate another valid key pair and take public one
  EXPECT_OUTCOME_TRUE(kp1, ed25519_provider->generateKeypair());
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
TEST_F(ED25519ProviderTest, DISABLED_VerifyInvalidKeyFail) {
  EXPECT_OUTCOME_TRUE(kp, ed25519_provider->generateKeypair());
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
TEST_F(ED25519ProviderTest, GenerateBySeedSuccess) {
  EXPECT_OUTCOME_TRUE(seed, ED25519Seed::fromHex(hex_seed));
  EXPECT_OUTCOME_TRUE(public_key, ED25519PublicKey::fromHex(hex_public_key));

  // private key is the same as seed
  EXPECT_OUTCOME_TRUE(private_key, ED25519PrivateKey::fromHex(hex_seed));

  EXPECT_OUTCOME_TRUE(kp, ed25519_provider->generateKeypair(seed));

  ASSERT_EQ(kp.private_key, private_key);
  ASSERT_EQ(kp.public_key, public_key);
}
