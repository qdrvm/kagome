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

class KeyTest : public testing::Test {
 public:
 protected:
  BoostRandomGenerator random_;
  KeyGeneratorImpl keygen_{random_};
};

/**
 * @given key generator
 * @when generateRsa of type RSA1024 is called
 * @then obtained key pair has type RSA1024
 */
TEST_F(KeyTest, generateRSA1024Success) {
  auto &&res = keygen_.generateRsa(RSAKeyType::RSA1024);
  ASSERT_TRUE(res);
  auto &&val = res.value();
  ASSERT_EQ(val.privateKey.type, Key::Type::RSA1024);
  ASSERT_EQ(val.publicKey.type, Key::Type::RSA1024);
}

/**
 * @given key generator
 * @when generateRsa of type RSA2048 is called
 * @then obtained key pair has type RSA2048
 */
TEST_F(KeyTest, generateRSA2048Success) {
  auto &&res = keygen_.generateRsa(RSAKeyType::RSA2048);
  ASSERT_TRUE(res);
  auto &&val = res.value();
  ASSERT_EQ(val.privateKey.type, Key::Type::RSA2048);
  ASSERT_EQ(val.publicKey.type, Key::Type::RSA2048);
}

/**
 * @given key generator
 * @when generateRsa of type RSA4096 is called
 * @then obtained key pair has type RSA4096
 */
TEST_F(KeyTest, generateRSA4096Success) {
  auto &&res = keygen_.generateRsa(RSAKeyType::RSA4096);
  ASSERT_TRUE(res);
  auto &&val = res.value();
  ASSERT_EQ(val.privateKey.type, Key::Type::RSA4096);
  ASSERT_EQ(val.publicKey.type, Key::Type::RSA4096);
}

/**
 * @given key generator instance
 * @when 2 keys of RSA1024 type are generated
 * @then these keys are different
 */
TEST_F(KeyTest, Rsa1024KeysNotSame) {
  auto &&res1 = keygen_.generateRsa(RSAKeyType::RSA1024);
  ASSERT_TRUE(res1);
  auto &&val1 = res1.value();
  auto &&res2 = keygen_.generateRsa(RSAKeyType::RSA1024);
  ASSERT_TRUE(res2);
  auto &&val2 = res2.value();
  ASSERT_NE(val1.privateKey.data.toVector(), val2.privateKey.data.toVector());
}

/**
 * @given key generator instance
 * @when generateEd25519 is called
 * @then obtained key pair have type ED25519
 */
TEST_F(KeyTest, generateED25519Success) {
  auto &&res = keygen_.generateEd25519();
  ASSERT_TRUE(res);
  auto &&val = res.value();
  ASSERT_EQ(val.privateKey.type, Key::Type::ED25519);
  ASSERT_EQ(val.publicKey.type, Key::Type::ED25519);
}

/**
 * @given key generator instance
 * @when 2 keys of SECP256K1 type are generated
 * @then these keys are different
 */
TEST_F(KeyTest, Ed25519KeysNotSame) {
  auto &&res1 = keygen_.generateEd25519();
  ASSERT_TRUE(res1);
  auto &&val1 = res1.value();
  auto &&res2 = keygen_.generateEd25519();
  ASSERT_TRUE(res2);
  auto &&val2 = res2.value();
  ASSERT_NE(val1.privateKey.data.toVector(), val2.privateKey.data.toVector());
}

/**
 * @given key generator instance
 * @when generateSecp256k1 is called
 * @then obtained key pair have type SECP256K1
 */
TEST_F(KeyTest, generateSecp256k1Success) {
  auto &&res = keygen_.generateSecp256k1();
  ASSERT_TRUE(res);
  auto &&val = res.value();
  ASSERT_EQ(val.privateKey.type, Key::Type::SECP256K1);
  ASSERT_EQ(val.publicKey.type, Key::Type::SECP256K1);

  std::cout << val.privateKey.data.toBytes() << std::endl;
  std::cout << val.publicKey.data.toBytes() << std::endl;
}

/**
 * @given key generator instance
 * @when 2 keys of SECP256K1 type are generated
 * @then these keys are different
 */
TEST_F(KeyTest, Secp256k1KeysNotSame) {
  auto &&res1 = keygen_.generateSecp256k1();
  ASSERT_TRUE(res1);
  auto &&val1 = res1.value();
  auto &&res2 = keygen_.generateSecp256k1();
  ASSERT_TRUE(res2);
  auto &&val2 = res2.value();
  ASSERT_NE(val1.privateKey.data.toVector(), val2.privateKey.data.toVector());
}