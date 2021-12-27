/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <gsl/span>

#include "crypto/ecdsa/ecdsa_provider_impl.hpp"
#include "crypto/random_generator/boost_generator.hpp"
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
    ecdsa_provider = std::make_shared<EcdsaProviderImpl>();

    std::string_view m = "i am a message";
    message = std::vector<uint8_t>(m.begin(), m.end());
    message_span = message;

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

  std::string_view hex_seed;
  std::string_view hex_public_key;

  gsl::span<uint8_t> message_span;
  std::vector<uint8_t> message;
  std::shared_ptr<EcdsaProvider> ecdsa_provider;
};

/**
 * @given ecdsa provider instance configured
 * @when generate 2 keypairs, repeat it 10 times
 * @then each time keys are different
 */
TEST_F(EcdsaProviderTest, GenerateKeysNotEqual) {
  for (auto i = 0; i < 10; ++i) {
    auto kp1 = ecdsa_provider->generate().value();
    auto kp2 = ecdsa_provider->generate().value();
    ASSERT_NE(kp1.public_key, kp2.public_key);
    ASSERT_NE(kp1.secret_key, kp2.secret_key);
  }
}

/**
 * @given ecdsa provider instance configured with predefined message
 * @when generate a keypair @and sign message
 * @and verify signed message with generated public key
 * @then verification succeeds
 */
TEST_F(EcdsaProviderTest, SignVerifySuccess) {
  auto kp = ecdsa_provider->generate().value();
  EXPECT_OUTCOME_TRUE(signature,
                      ecdsa_provider->sign(message_span, kp.secret_key));
  EXPECT_OUTCOME_TRUE(
      res, ecdsa_provider->verify(message_span, signature, kp.public_key));
  ASSERT_EQ(res, true);
}

/**
 * Don't try to sign a message using invalid key pair, this may lead to
 * program termination
 *
 * @given ecdsa provider instance configured with predefined message
 * @when generate a keypair @and make public key invalid @and sign message
 * @then sign fails
 */
TEST_F(EcdsaProviderTest, SignWithInvalidKeyFails) {
  auto kp = ecdsa_provider->generate().value();
  kp.secret_key.fill(1);
  EXPECT_OUTCOME_FALSE_1(ecdsa_provider->sign(message_span, kp.secret_key));
}

/**
 * @given ecdsa provider instance configured with predefined message
 * @when generate keypair @and sign message @and take another public key
 * @and verify signed message
 * @then verification succeeds, but verification result is false
 */
TEST_F(EcdsaProviderTest, VerifyWrongKeyFail) {
  auto kp = ecdsa_provider->generate().value();
  EXPECT_OUTCOME_TRUE(signature,
                      ecdsa_provider->sign(message_span, kp.secret_key));
  // generate another valid key pair and take public one
  auto kp1 = ecdsa_provider->generate().value();
  EXPECT_OUTCOME_TRUE(
      ver_res, ecdsa_provider->verify(message_span, signature, kp1.public_key));

  ASSERT_FALSE(ver_res);
}

/**
 * @given seed value
 * @when generate key pair by seed
 * @then public and private keys come up with predefined values
 */
TEST_F(EcdsaProviderTest, GenerateBySeedSuccess) {
  EXPECT_OUTCOME_TRUE(seed, EcdsaSeed::fromHex(hex_seed));
  EXPECT_OUTCOME_TRUE(public_key, EcdsaPublicKey::fromHex(hex_public_key));

  // private key is the same as seed
  EXPECT_OUTCOME_TRUE(private_key, EcdsaPrivateKey::fromHex(hex_seed));

  auto pk = ecdsa_provider->derive(seed).value();

  ASSERT_EQ(pk, public_key);
}
