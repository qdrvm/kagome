/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "crypto/sr25519/sr25519_provider_impl.hpp"

#include <gtest/gtest.h>
#include <gsl/span>
#include "crypto/random_generator/boost_generator.hpp"
#include "testutil/outcome.hpp"

using kagome::crypto::BoostRandomGenerator;
using kagome::crypto::CSPRNG;
using kagome::crypto::SR25519Provider;
using kagome::crypto::SR25519ProviderImpl;

struct SR25519ProviderTest : public ::testing::Test {
  void SetUp() override {
    random_generator = std::make_shared<BoostRandomGenerator>();
    sr25519_provider = std::make_shared<SR25519ProviderImpl>(random_generator);

    std::string_view m = "i am a message";
    message.clear();
    message.reserve(m.length());
    for (auto c : m) {
      message.push_back(c);
    }
  }

  gsl::span<uint8_t> message_span;
  std::vector<uint8_t> message;
  std::shared_ptr<CSPRNG> random_generator;
  std::shared_ptr<SR25519Provider> sr25519_provider;
};

/**
 * @given sr25519 provider instance configured with boost random generator
 * @when generate 2 keypairs, repeat it 10 times
 * @then each time keys are different
 */
TEST_F(SR25519ProviderTest, GenerateKeysNotEqual) {
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
TEST_F(SR25519ProviderTest, SignVerifySuccess) {
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
TEST_F(SR25519ProviderTest, DISABLED_SignWithInvalidKeyFails) {
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
TEST_F(SR25519ProviderTest, VerifyWrongKeyFail) {
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
TEST_F(SR25519ProviderTest, DISABLED_VerifyInvalidKeyFail) {
  auto kp = sr25519_provider->generateKeypair();
  EXPECT_OUTCOME_TRUE(signature, sr25519_provider->sign(kp, message_span));
  // make public key invalid
  kp.public_key.fill(1);
  EXPECT_OUTCOME_FALSE_1(
      sr25519_provider->verify(signature, message_span, kp.public_key));
}
