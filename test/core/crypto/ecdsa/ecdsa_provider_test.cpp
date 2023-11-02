/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <span>

#include "crypto/bip39/impl/bip39_provider_impl.hpp"
#include "crypto/ecdsa/ecdsa_provider_impl.hpp"
#include "crypto/hasher/hasher_impl.hpp"
#include "crypto/pbkdf2/impl/pbkdf2_provider_impl.hpp"
#include "crypto/random_generator/boost_generator.hpp"
#include "testutil/outcome.hpp"
#include "testutil/prepare_loggers.hpp"

using kagome::crypto::Bip39ProviderImpl;
using kagome::crypto::BoostRandomGenerator;
using kagome::crypto::EcdsaPrivateKey;
using kagome::crypto::EcdsaProvider;
using kagome::crypto::EcdsaProviderImpl;
using kagome::crypto::EcdsaPublicKey;
using kagome::crypto::EcdsaSeed;
using kagome::crypto::EcdsaSignature;
using kagome::crypto::HasherImpl;
using kagome::crypto::Pbkdf2ProviderImpl;

struct EcdsaProviderTest : public ::testing::Test {
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {
    ecdsa_provider_ = std::make_shared<EcdsaProviderImpl>(hasher);

    message = kagome::common::Buffer::fromString("i am a message").asVector();
  }

  auto generate() {
    EcdsaSeed seed;
    csprng->fillRandomly(seed);
    return ecdsa_provider_->generateKeypair(seed, {}).value();
  }

  std::shared_ptr<BoostRandomGenerator> csprng =
      std::make_shared<BoostRandomGenerator>();
  std::shared_ptr<HasherImpl> hasher = std::make_shared<HasherImpl>();
  std::shared_ptr<EcdsaProvider> ecdsa_provider_;

  std::vector<uint8_t> message;
};

/**
 * @given ecdsa provider instance configured
 * @when generate 2 keypairs, repeat it 10 times
 * @then each time keys are different
 */
TEST_F(EcdsaProviderTest, GenerateKeysNotEqual) {
  for (auto i = 0; i < 10; ++i) {
    auto kp1 = generate();
    auto kp2 = generate();
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
  auto key_pair = generate();
  EXPECT_OUTCOME_TRUE(signature,
                      ecdsa_provider_->sign(message, key_pair.secret_key));
  EXPECT_OUTCOME_TRUE(
      verify_res,
      ecdsa_provider_->verify(message, signature, key_pair.public_key));
  ASSERT_EQ(verify_res, true);
}

/**
 * @given ecdsa provider instance configured with predefined message
 * @when generate keypair @and sign message @and take another public key
 * @and verify signed message
 * @then verification succeeds, but verification result is false
 */
TEST_F(EcdsaProviderTest, VerifyWrongKeyFail) {
  auto key_pair = generate();
  EXPECT_OUTCOME_TRUE(signature,
                      ecdsa_provider_->sign(message, key_pair.secret_key));
  // generate another valid key pair and take public one
  auto another_keypair = generate();
  EXPECT_OUTCOME_TRUE(
      ver_res,
      ecdsa_provider_->verify(message, signature, another_keypair.public_key));

  ASSERT_FALSE(ver_res);
}

// polkadot key inspect --scheme ecdsa PHRASE
TEST_F(EcdsaProviderTest, Junctions) {
  Bip39ProviderImpl bip_provider{
      std::make_shared<Pbkdf2ProviderImpl>(),
      hasher,
  };
  auto f = [&](std::string_view phrase, std::string_view pub_str) {
    auto bip = bip_provider.generateSeed(phrase).value();
    auto keys =
        ecdsa_provider_
            ->generateKeypair(bip.as<EcdsaSeed>().value(), bip.junctions)
            .value();
    EXPECT_EQ(keys.public_key.toHex(), pub_str);
  };
  f("//Alice",
    "020a1091341fe5664bfa1782d5e04779689068c916b04cb365ec3153755684d9a1");
  f("//1234",
    "02f22d3c818ff50f22b5fcf5c76c84b1a4abbb8f3ac1d58b545bb5877a2e2521b9");
  f("", "035b26108e8b97479c547da4860d862dc08ab2c29ada449c74d5a9a58a6c46a8c4");
}

// https://github.com/paritytech/substrate/blob/6f0f5a92739b92199b3345fc4a716211c8a8b46f/primitives/core/src/ecdsa.rs#L551-L568
TEST_F(EcdsaProviderTest, Compatible) {
  auto seed =
      EcdsaSeed::fromHex(
          "9d61b19deffd5a60ba844af492ec2cc44449c5697b326919703bac031cae7f60")
          .value();
  auto keys = ecdsa_provider_->generateKeypair(seed, {}).value();
  auto sig =
      EcdsaSignature::fromHex(
          "3dde91174bd9359027be59a428b8146513df80a2a3c7eda2194f64de04a69ab97b75"
          "3169e94db6ffd50921a2668a48b94ca11e3d32c1ff19cfe88890aa7e8f3c00")
          .value();
  EXPECT_TRUE(ecdsa_provider_->verify({}, sig, keys.public_key).value());
}
