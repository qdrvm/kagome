/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/bip39/impl/bip39_provider_impl.hpp"
#include "crypto/crypto_store/crypto_store_impl.hpp"
#include "crypto/ed25519/ed25519_provider_impl.hpp"
#include "crypto/pbkdf2/impl/pbkdf2_provider_impl.hpp"
#include "crypto/random_generator/boost_generator.hpp"
#include "crypto/secp256k1/secp256k1_provider_impl.hpp"
#include "crypto/sr25519/sr25519_provider_impl.hpp"

#include "testutil/outcome.hpp"
#include "testutil/storage/base_fs_test.hpp"

using kagome::common::Blob;
using kagome::common::Buffer;
using kagome::crypto::Bip39Provider;
using kagome::crypto::Bip39ProviderImpl;
using kagome::crypto::BoostRandomGenerator;
using kagome::crypto::CryptoStore;
using kagome::crypto::CryptoStoreError;
using kagome::crypto::CryptoStoreImpl;
using kagome::crypto::ED25519Keypair;
using kagome::crypto::ED25519Provider;
using kagome::crypto::ED25519ProviderImpl;
using kagome::crypto::KeyTypeId;
using kagome::crypto::Pbkdf2Provider;
using kagome::crypto::Pbkdf2ProviderImpl;
using kagome::crypto::Secp256k1Provider;
using kagome::crypto::Secp256k1ProviderImpl;
using kagome::crypto::SR25519Keypair;
using kagome::crypto::SR25519Provider;
using kagome::crypto::SR25519ProviderImpl;

using namespace kagome::crypto::key_types;

CryptoStoreImpl::Path crypto_store_test_directory =
    boost::filesystem::temp_directory_path().append("crypto_store_test");

struct CryptoStoreTest : public test::BaseFS_Test {
  CryptoStoreTest() : BaseFS_Test(crypto_store_test_directory) {}

  void SetUp() override {
    auto ed25519_provider = std::make_shared<ED25519ProviderImpl>();
    auto csprng = std::make_shared<BoostRandomGenerator>();
    auto sr25519_provider = std::make_shared<SR25519ProviderImpl>(csprng);
    auto secp256k1_provider = std::make_shared<Secp256k1ProviderImpl>();

    auto pbkdf2_provider = std::make_shared<Pbkdf2ProviderImpl>();
    auto bip39_provider =
        std::make_shared<Bip39ProviderImpl>(std::move(pbkdf2_provider));
    crypto_store =
        std::make_shared<CryptoStoreImpl>(std::move(ed25519_provider),
                                          std::move(sr25519_provider),
                                          std::move(secp256k1_provider),
                                          std::move(bip39_provider),
                                          std::move(csprng));

    mnemonic =
        "ozone drill grab fiber curtain grace pudding thank cruise elder eight "
        "picnic";
    EXPECT_OUTCOME_TRUE(e, Buffer::fromHex("9e885d952ad362caeb4efe34a8e91bd2"));
    entropy = std::move(e);
    EXPECT_OUTCOME_TRUE(
        s,
        Blob<64>::fromHex(
            "f4956be6960bc145cdab782e649a5056598fd07cd3f32ceb73421c3da278332413"
            "24dc2c8b0a4d847eee457e6d4c5429f5e625ece22abaa6a976e82f1ec5531d"));
    seed = std::move(s);
    key_type = kBabe;
  }

  std::shared_ptr<CryptoStore> crypto_store;
  std::string mnemonic;
  Buffer entropy;
  Blob<64> seed;
  KeyTypeId key_type;

  ED25519Keypair ed_pair;
  SR25519Keypair sr_pair;
};

/**
 * @given cryptostore instance @and non-existant, but valid directory path
 * @when call initialize using provided directory as argument
 * @then initialization succeeds @and corresponding directory is created
 */
TEST_F(CryptoStoreTest, InitializeSuccess) {
  auto path = crypto_store_test_directory.append("aaa");
  ASSERT_FALSE(boost::filesystem::exists(path));
  EXPECT_OUTCOME_TRUE_MSG_1(crypto_store->initialize(path),
                            "initialization failed");
  ASSERT_TRUE(boost::filesystem::exists(path));
}

/**
 * @given cryptostore instance, type, mnemonic and predefined key pair
 * @when generateEd25519Keypair is called
 * @then method call succeeds and result matches predefined key pair
 */
TEST_F(CryptoStoreTest, generateEd25519KeypairMnemonicSuccess) {
  auto path = crypto_store_test_directory.append("aaa");
  ASSERT_FALSE(boost::filesystem::exists(path));
  EXPECT_OUTCOME_TRUE_MSG_1(crypto_store->initialize(path),
                            "initialization failed");

  EXPECT_OUTCOME_FALSE(
      err, crypto_store->findEd25519Keypair(key_type, ed_pair.public_key));
  ASSERT_EQ(err, CryptoStoreError::KEY_NOT_FOUND);

  EXPECT_OUTCOME_TRUE(pair,
                      crypto_store->generateEd25519Keypair(key_type, mnemonic));

  // check that created pair is now contained in memory
  EXPECT_OUTCOME_TRUE(
      found, crypto_store->findEd25519Keypair(key_type, pair.public_key));
  ASSERT_EQ(found, pair);
}

/**
 * @given cryptostore instance, type, mnemonic and predefined key pair
 * @when generateSr25519Keypair is called
 * @then method call succeeds and result matches predefined key pair
 */
TEST_F(CryptoStoreTest, generateSr25519KeypairMnemonicSuccess) {
  auto path = crypto_store_test_directory.append("aaa");
  ASSERT_FALSE(boost::filesystem::exists(path));
  EXPECT_OUTCOME_TRUE_MSG_1(crypto_store->initialize(path),
                            "initialization failed");

  EXPECT_OUTCOME_FALSE(
      err, crypto_store->findSr25519Keypair(key_type, ed_pair.public_key));
  ASSERT_EQ(err, CryptoStoreError::KEY_NOT_FOUND);

  EXPECT_OUTCOME_TRUE(pair,
                      crypto_store->generateSr25519Keypair(key_type, mnemonic));

  // check that created pair is now contained in memory
  EXPECT_OUTCOME_TRUE(
      found, crypto_store->findSr25519Keypair(key_type, pair.public_key));
  ASSERT_EQ(found, pair);
}

///**
// * @given
// * @when
// * @then
// */
// TEST_F(CryptoStoreTest, generateEd25519KeypairSeedSuccess) {}
//
///**
// * @given
// * @when
// * @then
// */
// TEST_F(CryptoStoreTest, generateSr25519KeypairSeedSuccess) {}
//
///**
// * @given
// * @when
// * @then
// */
// TEST_F(CryptoStoreTest, generateEd25519KeypairStoreSuccess) {}
//
///**
// * @given
// * @when
// * @then
// */
// TEST_F(CryptoStoreTest, generateSr25519KeypairStoreSuccess) {}
//
///**
// * @given
// * @when
// * @then
// */
// TEST_F(CryptoStoreTest, findEd25519KeypairSuccess) {}
//
///**
// * @given
// * @when
// * @then
// */
// TEST_F(CryptoStoreTest, findSr25519KeypairSuccess) {}
//
///**
// * @given
// * @when
// * @then
// */
// TEST_F(CryptoStoreTest, getEd25519PublicKeysSuccess) {}
//
///**
// * @given
// * @when
// * @then
// */
// TEST_F(CryptoStoreTest, getSr25519PublicKeysSuccess) {}
