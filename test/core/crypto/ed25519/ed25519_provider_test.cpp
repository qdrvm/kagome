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
 * @given sr25519 provider instance configured with boost random generator
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
