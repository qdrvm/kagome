/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/ed25519/ed25519_provider_impl.hpp"

#include <gtest/gtest.h>
#include <gsl/span>
#include "testutil/outcome.hpp"

using kagome::crypto::ED25519Provider;
using kagome::crypto::ED25519ProviderImpl;

struct ED25519ProviderTest : public ::testing::Test {
  void SetUp() override {
    ed25519_provider = std::make_shared<ED25519ProviderImpl>();

    std::string_view m = "i am a message";
    message.clear();
    message.reserve(m.length());
    for (auto c : m) {
      message.push_back(c);
    }
  }

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
