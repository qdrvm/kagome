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

using libp2p::crypto::Key;
using libp2p::crypto::KeyGeneratorImpl;
using libp2p::crypto::common::RSAKeyType;
using libp2p::crypto::random::BoostRandomGenerator;

class RsaTest : public testing::Test {
 protected:
  void SetUp() override {
    keygen_.initialize(random_);
  }

  BoostRandomGenerator random_;
  KeyGeneratorImpl keygen_;
};

/**
 * @given key generator
 * @when generateRsa of type RSA1024 is called
 * @then obtained key pair have type RSA1024
 */
TEST_F(RsaTest, generate1024Success) {
  auto &&res = keygen_.generateRsa(RSAKeyType::RSA1024);
  ASSERT_TRUE(res);
  ASSERT_EQ(res.value().privateKey.type, Key::Type::RSA1024);
  ASSERT_EQ(res.value().publicKey.type, Key::Type::RSA1024);
  std::cout << res.value().privateKey.data.toBytes() << std::endl;
  std::cout << res.value().publicKey.data.toBytes() << std::endl;
}

/**
 * @given key generator
 * @when generateRsa of type RSA2048 is called
 * @then obtained key pair have type RSA2048
 */
TEST_F(RsaTest, generate2048Success) {
  auto &&res = keygen_.generateRsa(RSAKeyType::RSA2048);
  ASSERT_TRUE(res);
  ASSERT_EQ(res.value().privateKey.type, Key::Type::RSA2048);
  ASSERT_EQ(res.value().publicKey.type, Key::Type::RSA2048);
  std::cout << res.value().privateKey.data.toBytes() << std::endl;
  std::cout << res.value().publicKey.data.toBytes() << std::endl;
}

/**
 * @given key generator
 * @when generateRsa of type RSA4096 is called
 * @then obtained key pair have type RSA4096
 */
TEST_F(RsaTest, generate4096Success) {
  auto &&res = keygen_.generateRsa(RSAKeyType::RSA4096);
  ASSERT_TRUE(res);
  ASSERT_EQ(res.value().privateKey.type, Key::Type::RSA4096);
  ASSERT_EQ(res.value().publicKey.type, Key::Type::RSA4096);
  std::cout << res.value().privateKey.data.toBytes() << std::endl;
  std::cout << res.value().publicKey.data.toBytes() << std::endl;
}

/**
 * @given key generator instance
 * @when 2 keys of the same type are generated
 * @then these keys are different
 */
TEST_F(RsaTest, Rsa1024KeysNotSame) {
  auto &&res1 = keygen_.generateRsa(RSAKeyType::RSA1024);
  ASSERT_TRUE(res1);
  auto &&res2 = keygen_.generateRsa(RSAKeyType::RSA1024);
  ASSERT_TRUE(res2);
  ASSERT_NE(res1.value().privateKey.data.toVector(), res2.value().privateKey.data.toVector());
}
