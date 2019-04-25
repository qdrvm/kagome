/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/crypto/key_generator/key_generator_impl.hpp"

#include <gtest/gtest.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <gsl/gsl_util>
#include "libp2p/crypto/random/impl/boost_generator.hpp"

using libp2p::crypto::random::BoostRandomGenerator;
using libp2p::crypto::KeyGeneratorImpl;
using libp2p::crypto::common::RSAKeyType;
using libp2p::crypto::common::KeyType;

class RsaTest : public testing::Test {
 protected:
  void SetUp() override {
    keygen_.initialize(random_);
  }

  BoostRandomGenerator random_;
  KeyGeneratorImpl keygen_;
};

/**
 * @given
 * @when
 * @then
 */
TEST_F(RsaTest, generate1024Success) {
  auto && res = keygen_.generateRsa(RSAKeyType::RSA1024);
  ASSERT_TRUE(res);
  ASSERT_EQ(res.value().private_key->getType(), KeyType::RSA1024);
  ASSERT_EQ(res.value().public_key->getType(), KeyType::RSA1024);
  std::cout << res.value().private_key->getBytes().toHex() << std::endl;
}

/**
 * @given
 * @when
 * @then
 */
TEST_F(RsaTest, DISABLED_generate2048Success) {
  FAIL();
}

/**
 * @given
 * @when
 * @then
 */
TEST_F(RsaTest, DISABLED_generate4096Success) {
  FAIL();
}
