/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/crypto/key_generator/key_generator_impl.hpp"

#include <gtest/gtest.h>
#include <boost/filesystem.hpp>
#include <gsl/gsl_util>
#include "libp2p/crypto/error.hpp"
#include "libp2p/crypto/random_generator/boost_generator.hpp"
#include "testutil/outcome.hpp"

using kagome::common::Buffer;
using libp2p::crypto::Key;
using libp2p::crypto::KeyGeneratorError;
using libp2p::crypto::KeyGeneratorImpl;
using libp2p::crypto::common::RSAKeyType;
using libp2p::crypto::random::BoostRandomGenerator;


class KeyTest : public virtual testing::Test {
 protected:
  BoostRandomGenerator random_;
  KeyGeneratorImpl keygen_{random_};
};

class GenerateKeyTest : public KeyTest {
 public:
  /**
   * @brief tests generating key pair against specified key type
   * @param key_type specified key type
   */
  void testGenerateKey(Key::Type key_type) {
    EXPECT_OUTCOME_TRUE_2(val, keygen_.generateKeys(key_type));
    ASSERT_EQ(val.privateKey.type, key_type);
    ASSERT_EQ(val.publicKey.type, key_type);
  }

  /**
   * @brief ensures that 2 generated key pairs of the same type
   * are different
   * @param key_type specified key type
   */
  void testKeysAreDifferent(Key::Type key_type) {
    EXPECT_OUTCOME_TRUE_2(val1, keygen_.generateKeys(key_type));
    EXPECT_OUTCOME_TRUE_2(val2, keygen_.generateKeys(key_type));
    ASSERT_NE(val1.privateKey.data.toVector(), val2.privateKey.data.toVector());
    ASSERT_NE(val1.publicKey.data.toVector(), val2.privateKey.data.toVector());
  }
};

class DeriveKeyTest : public KeyTest {
 protected:
  void testDerivePublicKey(Key::Type key_type) {
    EXPECT_OUTCOME_TRUE_2(keys, keygen_.generateKeys(key_type));
    EXPECT_OUTCOME_TRUE_2(derived, keygen_.derivePublicKey(keys.privateKey));
    ASSERT_EQ(derived.type, key_type);
    ASSERT_EQ(keys.publicKey.data.toVector(), derived.data.toVector());
  }
};



/**
 * @given key generator
 * @when generateRsa of type RSA1024 is called
 * @then obtained key pair has type RSA1024
 */
TEST_F(GenerateKeyTest, GenerateRSA1024Success) {
  testGenerateKey(Key::Type::RSA1024);
}

/**
 * @given key generator
 * @when generateRsa of type RSA2048 is called
 * @then obtained key pair has type RSA2048
 */
TEST_F(GenerateKeyTest, GenerateRSA2048Success) {
  testGenerateKey(Key::Type::RSA2048);
}

/**
 * @given key generator
 * @when generateRsa of type RSA4096 is called
 * @then obtained key pair has type RSA4096
 */
TEST_F(GenerateKeyTest, GenerateRSA4096Success) {
  testGenerateKey(Key::Type::RSA4096);
}

/**
 * @given key generator instance
 * @when 2 keys of RSA1024 type are generated
 * @then these keys are different
 */
TEST_F(GenerateKeyTest, Rsa1024KeysNotSame) {
  testKeysAreDifferent(Key::Type::RSA1024);
}

/**
 * @given key generator instance
 * @when generateEd25519 is called
 * @then obtained key pair have type ED25519
 */
TEST_F(GenerateKeyTest, GenerateED25519Success) {
  testGenerateKey(Key::Type::ED25519);
}

/**
 * @given key generator instance
 * @when 2 keys of SECP256K1 type are generated
 * @then these keys are different
 */
TEST_F(GenerateKeyTest, Ed25519KeysNotSame) {
  testKeysAreDifferent(Key::Type::ED25519);
}

/**
 * @given key generator instance
 * @when generateSecp256k1 is called
 * @then obtained key pair have type SECP256K1
 */
TEST_F(GenerateKeyTest, GenerateSecp256k1Success) {
  testGenerateKey(Key::Type::SECP256K1);
}

/**
 * @given key generator instance
 * @when 2 keys of SECP256K1 type are generated
 * @then these keys are different
 */
TEST_F(GenerateKeyTest, Secp256k1KeysNotSame) {
  testKeysAreDifferent(Key::Type::SECP256K1);
}

/**
 * @given RSA 1024 bits private key
 * @when derivePublicKey is called
 * @then operation succeeds
 */
TEST_F(DeriveKeyTest, DeriveRsa1024Successful) {
  testDerivePublicKey(Key::Type::RSA1024);
}

/**
 * @given RSA 2048 bits private key
 * @when derivePublicKey is called
 * @then operation succeeds
 */
TEST_F(DeriveKeyTest, DeriveRsa2048Successful) {
  testDerivePublicKey(Key::Type::RSA2048);
}

/**
 * @given RSA 4096 bits private key
 * @when derivePublicKey is called
 * @then operation succeeds
 */
TEST_F(DeriveKeyTest, DeriveRsa4096Successful) {
  testDerivePublicKey(Key::Type::RSA4096);
}

/**
 * @given ED25519 private key
 * @when derivePublicKey is called
 * @then operation succeeds
 */
TEST_F(DeriveKeyTest, DeriveEd25519uccessful) {
  testDerivePublicKey(Key::Type::ED25519);
}

/**
 * @given SECP256k1 private key
 * @when derivePublicKey is called
 * @then operation succeeds
 */
TEST_F(DeriveKeyTest, DeriveSecp2561kSuccessful) {
  testDerivePublicKey(Key::Type::SECP256K1);
}
