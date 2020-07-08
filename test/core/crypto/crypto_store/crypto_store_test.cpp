/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/crypto_store/crypto_store_impl.hpp"

#include "crypto/bip39/impl/bip39_provider_impl.hpp"
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
using kagome::crypto::ED25519PrivateKey;
using kagome::crypto::ED25519Provider;
using kagome::crypto::ED25519ProviderImpl;
using kagome::crypto::ED25519PublicKey;
using kagome::crypto::KeyTypeId;
using kagome::crypto::Pbkdf2Provider;
using kagome::crypto::Pbkdf2ProviderImpl;
using kagome::crypto::Secp256k1Provider;
using kagome::crypto::Secp256k1ProviderImpl;
using kagome::crypto::SR25519Keypair;
using kagome::crypto::SR25519Provider;
using kagome::crypto::SR25519ProviderImpl;
using kagome::crypto::SR25519PublicKey;
using kagome::crypto::SR25519SecretKey;

using namespace kagome::crypto::key_types;

static CryptoStoreImpl::Path crypto_store_test_directory =
    boost::filesystem::temp_directory_path() / "crypto_store_test";

struct CryptoStoreTest : public test::BaseFS_Test {
  CryptoStoreTest() : BaseFS_Test(crypto_store_test_directory) {}

  void SetUp() override {
    auto ed25519_provider = std::make_shared<ED25519ProviderImpl>();
    auto csprng = std::make_shared<BoostRandomGenerator>();
    auto sr25519_provider = std::make_shared<SR25519ProviderImpl>(csprng);
    auto secp256k1_provider = std::make_shared<Secp256k1ProviderImpl>();

    auto pbkdf2_provider = std::make_shared<Pbkdf2ProviderImpl>();
    bip39_provider =
        std::make_shared<Bip39ProviderImpl>(std::move(pbkdf2_provider));
    crypto_store =
        std::make_shared<CryptoStoreImpl>(std::move(ed25519_provider),
                                          std::move(sr25519_provider),
                                          std::move(secp256k1_provider),
                                          bip39_provider,
                                          std::move(csprng));

    mnemonic =
        "ozone drill grab fiber curtain grace pudding thank cruise elder eight "
        "picnic";
    EXPECT_OUTCOME_TRUE(e, Buffer::fromHex("9e885d952ad362caeb4efe34a8e91bd2"));
    entropy = std::move(e);
    EXPECT_OUTCOME_TRUE(s,
                        Blob<32>::fromHex("a4681403ba5b6a3f3bd0b0604ce439a78244"
                                          "c7d43b127ec35cd8325602dd47fd"));
    seed = s;
    key_type = kBabe;

    EXPECT_OUTCOME_TRUE(
        ed_publ,
        ED25519PublicKey::fromHex("3e765f2bde3daadd443097b3145abf1f71f99f0aa946"
                                  "960990fe02aa26b7fc72"));
    EXPECT_OUTCOME_TRUE(
        ed_priv,
        ED25519PrivateKey::fromHex("a4681403ba5b6a3f3bd0b0604ce439a78244c7d43b1"
                                   "27ec35cd8325602dd47fd"));
    ed_pair = {ed_priv, ed_publ};

    EXPECT_OUTCOME_TRUE(
        sr_publ,
        SR25519PublicKey::fromHex("56a03c8afc0e7a3a8b1d53bcc875ba5b6364754f9045"
                                  "16009b57ef3adf96f61f"));
    EXPECT_OUTCOME_TRUE(
        sr_secr,
        SR25519SecretKey::fromHex(
            "ec96cb0816b67b045baae21841952a61ecb0612a109293e10c5453b950659c0a8b"
            "35b6d6196f33169334e36a05d624d9996d07243f9f71e638e3bc29a5330ec9"));
    sr_pair = {sr_secr, sr_publ};

    EXPECT_OUTCOME_TRUE_MSG_1(
        crypto_store->initialize(crypto_store_test_directory),
        "initialization failed");
  }

  bool isStoredOnDisk(KeyTypeId kt, const Blob<32> &public_key) {
    auto file_name = kagome::crypto::decodeKeyTypeId(kt) + public_key.toHex();
    auto file_path = crypto_store_test_directory / file_name;
    return boost::filesystem::exists(file_path);
  }

  std::shared_ptr<Bip39Provider> bip39_provider;
  std::shared_ptr<CryptoStoreImpl> crypto_store;
  std::string mnemonic;
  Buffer entropy;
  Blob<32> seed;
  KeyTypeId key_type;

  ED25519Keypair ed_pair;
  SR25519Keypair sr_pair;
};

/**
 * @given cryptostore instance, type, mnemonic and predefined key pair
 * @when generateEd25519Keypair is called
 * @then method call succeeds and result matches predefined key pair
 * @and generated key pair is stored in memory
 */
TEST_F(CryptoStoreTest, generateEd25519KeypairMnemonicSuccess) {
  EXPECT_OUTCOME_FALSE(
      err, crypto_store->findEd25519Keypair(key_type, ed_pair.public_key));
  ASSERT_EQ(err, CryptoStoreError::KEY_NOT_FOUND);

  EXPECT_OUTCOME_TRUE(pair,
                      crypto_store->generateEd25519Keypair(key_type, mnemonic));
  ASSERT_EQ(pair, ed_pair);

  // check that created pair is now contained in memory
  EXPECT_OUTCOME_TRUE(
      found, crypto_store->findEd25519Keypair(key_type, pair.public_key));
  ASSERT_EQ(found, pair);

  // not stored on disk
  ASSERT_FALSE(isStoredOnDisk(key_type, pair.public_key));
}

/**
 * @given cryptostore instance, type, mnemonic and predefined key pair
 * @when generateSr25519Keypair is called
 * @then method call succeeds and result matches predefined key pair
 * @and generated key pair is stored in memory
 */
TEST_F(CryptoStoreTest, generateSr25519KeypairMnemonicSuccess) {
  EXPECT_OUTCOME_FALSE(
      err, crypto_store->findSr25519Keypair(key_type, ed_pair.public_key));
  ASSERT_EQ(err, CryptoStoreError::KEY_NOT_FOUND);

  EXPECT_OUTCOME_TRUE(pair,
                      crypto_store->generateSr25519Keypair(key_type, mnemonic));
  ASSERT_EQ(pair, sr_pair);

  // check that created pair is now contained in memory
  EXPECT_OUTCOME_TRUE(
      found, crypto_store->findSr25519Keypair(key_type, pair.public_key));
  ASSERT_EQ(found, pair);
  // not stored on disk
  ASSERT_FALSE(isStoredOnDisk(key_type, pair.public_key));
}

/**
 * @given cryptostore instance, type, seed and predefined key pair
 * @when generateEd25519Keypair is called
 * @then method call succeeds and result matches predefined key pair
 * @and generated key pair is stored in memory
 */
TEST_F(CryptoStoreTest, generateEd25519KeypairSeedSuccess) {
  EXPECT_OUTCOME_FALSE(
      err, crypto_store->findEd25519Keypair(key_type, ed_pair.public_key));
  ASSERT_EQ(err, CryptoStoreError::KEY_NOT_FOUND);

  auto &&pair = crypto_store->generateEd25519Keypair(key_type, seed);
  ASSERT_EQ(pair, ed_pair);

  // check that created pair is now contained in memory
  EXPECT_OUTCOME_TRUE(
      found, crypto_store->findEd25519Keypair(key_type, pair.public_key));
  ASSERT_EQ(found, pair);
  // not stored on disk
  ASSERT_FALSE(isStoredOnDisk(key_type, pair.public_key));
}

/**
 * @given cryptostore instance, type, seed and predefined key pair
 * @when generateSr25519Keypair is called
 * @then method call succeeds and result matches predefined key pair
 * @and key generated pair is stored in memory
 */
TEST_F(CryptoStoreTest, generateSr25519KeypairSeedSuccess) {
  EXPECT_OUTCOME_FALSE(
      err, crypto_store->findSr25519Keypair(key_type, sr_pair.public_key));
  ASSERT_EQ(err, CryptoStoreError::KEY_NOT_FOUND);

  auto &&pair = crypto_store->generateSr25519Keypair(key_type, seed);
  ASSERT_EQ(pair, sr_pair);

  // check that created pair is now contained in memory
  EXPECT_OUTCOME_TRUE(
      found, crypto_store->findSr25519Keypair(key_type, pair.public_key));
  ASSERT_EQ(found, pair);

  // not stored on disk
  ASSERT_FALSE(isStoredOnDisk(key_type, pair.public_key));
}

/**
 * @given cryptostore instance, and key type
 * @when call generateEd25519Keypair(key_type)
 * @then a new ed25519 key pair is generated and stored on disk
 */
TEST_F(CryptoStoreTest, generateEd25519KeypairStoreSuccess) {
  EXPECT_OUTCOME_TRUE(pair, crypto_store->generateEd25519Keypair(key_type));

  // check that created pair is contained in the storage on disk
  EXPECT_OUTCOME_TRUE(
      found, crypto_store->findEd25519Keypair(key_type, pair.public_key));
  ASSERT_EQ(found, pair);

  // stored on disk
  ASSERT_TRUE(isStoredOnDisk(key_type, pair.public_key));
}

/**
 * @given cryptostore instance, and key type
 * @when call generateSr25519Keypair(key_type)
 * @then a new ed25519 key pair is generated and stored on disk
 */
TEST_F(CryptoStoreTest, generateSr25519KeypairStoreSuccess) {
  EXPECT_OUTCOME_TRUE(pair, crypto_store->generateSr25519Keypair(key_type));

  // check that created pair is contained in the storage on disk
  EXPECT_OUTCOME_TRUE(
      found, crypto_store->findSr25519Keypair(key_type, pair.public_key));
  ASSERT_EQ(found, pair);

  // stored on disk
  ASSERT_TRUE(isStoredOnDisk(key_type, pair.public_key));
}

/**
 * @given cryptostore instance, and key type
 * @when call getEd25519PublicKeys
 * @then collection of all ed25519 public keys of provided type is returned
 */
TEST_F(CryptoStoreTest, getEd25519PublicKeysSuccess) {
  EXPECT_OUTCOME_TRUE(pair1, crypto_store->generateEd25519Keypair(kBabe));
  EXPECT_OUTCOME_TRUE(pair2, crypto_store->generateEd25519Keypair(kBabe));
  EXPECT_OUTCOME_SUCCESS(pair3, crypto_store->generateEd25519Keypair(kLp2p));
  EXPECT_OUTCOME_SUCCESS(pair4, crypto_store->generateSr25519Keypair(kBabe));
  EXPECT_OUTCOME_SUCCESS(pair5, crypto_store->generateSr25519Keypair(kAcco));

  std::set<ED25519PublicKey> ed_babe_keys_set = {pair1.public_key,
                                                 pair2.public_key};
  std::vector<ED25519PublicKey> ed_babe_keys(ed_babe_keys_set.begin(),
                                             ed_babe_keys_set.end());

  auto &&keys = crypto_store->getEd25519PublicKeys(kBabe);
  ASSERT_EQ(ed_babe_keys, keys);
}

/**
 * @given cryptostore instance, and key type
 * @when call getSr25519PublicKeys
 * @then collection of all sr25519 public keys of provided type is returned
 */
TEST_F(CryptoStoreTest, getSr25519PublicKeysSuccess) {
  EXPECT_OUTCOME_TRUE(pair1, crypto_store->generateSr25519Keypair(kBabe));
  EXPECT_OUTCOME_TRUE(pair2, crypto_store->generateSr25519Keypair(kBabe));
  EXPECT_OUTCOME_SUCCESS(pair3, crypto_store->generateSr25519Keypair(kLp2p));
  EXPECT_OUTCOME_SUCCESS(pair4, crypto_store->generateEd25519Keypair(kBabe));
  EXPECT_OUTCOME_SUCCESS(pair5, crypto_store->generateEd25519Keypair(kAcco));

  std::set<SR25519PublicKey> sr_babe_keys_set = {pair1.public_key,
                                                 pair2.public_key};
  std::vector<SR25519PublicKey> sr_babe_keys(sr_babe_keys_set.begin(),
                                             sr_babe_keys_set.end());

  auto &&keys = crypto_store->getSr25519PublicKeys(kBabe);
  ASSERT_EQ(sr_babe_keys, keys);
}
