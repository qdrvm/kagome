/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <gsl/span>

#include "crypto/ecdsa/ecdsa_provider_impl.hpp"
#include "testutil/outcome.hpp"
#include "testutil/prepare_loggers.hpp"

using kagome::crypto::EcdsaPrivateKey;
using kagome::crypto::EcdsaProvider;
using kagome::crypto::EcdsaProviderImpl;
using kagome::crypto::EcdsaPublicKey;
using kagome::crypto::EcdsaSeed;

struct EcdsaProviderTest : public ::testing::Test {
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {
    ecdsa_provider_ = std::make_shared<EcdsaProviderImpl>();

    message = kagome::common::Buffer::fromString("i am a message").asVector();

    hex_seed =
        "3077020101042005a66b7bb0a232a51634f15cf7bbfc13885d3c176f91251d368acff9"
        "b14e4206a00a06082a8648ce3d030107a144034200048d8fc7919476b60c44cd32f71c"
        "69ef8a16c3febf757dbb77d410604f7014f0e8f5c4435708893015dcd4d7363f637d2f"
        "e6d9c9ea00fd1f2d1bb3fb8807f098f6";
    hex_public_key =
        "3059301306072a8648ce3d020106082a8648ce3d030107034200048d8fc7919476b60c"
        "44cd32f71c69ef8a16c3febf757dbb77d410604f7014f0e8f5c4435708893015dcd4d7"
        "363f637d2fe6d9c9ea00fd1f2d1bb3fb8807f098f6";
  }

  std::shared_ptr<EcdsaProvider> ecdsa_provider_;

  std::string_view hex_seed;
  std::string_view hex_public_key;

  std::vector<uint8_t> message;
};

/**
 * @given ecdsa provider instance configured
 * @when generate 2 keypairs, repeat it 10 times
 * @then each time keys are different
 */
TEST_F(EcdsaProviderTest, GenerateKeysNotEqual) {
  for (auto i = 0; i < 10; ++i) {
    EXPECT_OUTCOME_TRUE(kp1, ecdsa_provider_->generate());
    EXPECT_OUTCOME_TRUE(kp2, ecdsa_provider_->generate());
    ASSERT_NE(kp1.public_key, kp2.public_key);
    ASSERT_NE(kp1.secret_key, kp2.secret_key);
  }
}

/**
 * @given a keypair
 * @when a message gets signed
 * @then the signature verification against the key succeeds
 */
TEST_F(EcdsaProviderTest, SignVerifySuccess) {
  EXPECT_OUTCOME_TRUE(key_pair, ecdsa_provider_->generate());
  EXPECT_OUTCOME_TRUE(signature,
                      ecdsa_provider_->sign(message, key_pair.secret_key));
  EXPECT_OUTCOME_TRUE(
      verify_res,
      ecdsa_provider_->verify(message, signature, key_pair.public_key));
  ASSERT_EQ(verify_res, true);
}

/**
 * @given an ill-formed private key
 * @when the key is used to sign a message
 * @then no valid signature could be produced
 */
TEST_F(EcdsaProviderTest, SignWithInvalidKeyFails) {
  EcdsaPrivateKey key;
  key.fill(1);
  EXPECT_OUTCOME_FALSE_1(ecdsa_provider_->sign(message, key));
}

/**
 * @given ecdsa provider instance configured with predefined message
 * @when generate keypair @and sign message @and take another public key
 * @and verify signed message
 * @then verification succeeds, but verification result is false
 */
TEST_F(EcdsaProviderTest, VerifyWrongKeyFail) {
  auto key_pair = ecdsa_provider_->generate().value();
  EXPECT_OUTCOME_TRUE(signature,
                      ecdsa_provider_->sign(message, key_pair.secret_key));
  // generate another valid key pair and take public one
  auto another_keypair = ecdsa_provider_->generate().value();
  EXPECT_OUTCOME_TRUE(
      ver_res,
      ecdsa_provider_->verify(message, signature, another_keypair.public_key));

  ASSERT_FALSE(ver_res);
}

/**
 * @given a private key or seed
 * @when public key is derived
 * @then the resulting key matches the reference one
 */
TEST_F(EcdsaProviderTest, GenerateBySeedSuccess) {
  EXPECT_OUTCOME_TRUE(seed, EcdsaSeed::fromHex(hex_seed));
  EXPECT_OUTCOME_TRUE(public_key, EcdsaPublicKey::fromHex(hex_public_key));
  EXPECT_OUTCOME_TRUE(derived_key, ecdsa_provider_->derive(seed));

  ASSERT_EQ(derived_key, public_key);
}
