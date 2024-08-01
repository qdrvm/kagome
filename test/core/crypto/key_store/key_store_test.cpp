/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/key_store/key_store_impl.hpp"

#include <gmock/gmock.h>
#include <optional>

#include "crypto/bandersnatch/bandersnatch_provider_impl.hpp"
#include "crypto/bip39/impl/bip39_provider_impl.hpp"
#include "crypto/ecdsa/ecdsa_provider_impl.hpp"
#include "crypto/ed25519/ed25519_provider_impl.hpp"
#include "crypto/hasher/hasher_impl.hpp"
#include "crypto/pbkdf2/impl/pbkdf2_provider_impl.hpp"
#include "crypto/random_generator/boost_generator.hpp"
#include "crypto/sr25519/sr25519_provider_impl.hpp"

#include "mock/core/application/app_state_manager_mock.hpp"

#include "testutil/outcome.hpp"
#include "testutil/prepare_loggers.hpp"
#include "testutil/storage/base_fs_test.hpp"

using kagome::common::Blob;
using kagome::common::Buffer;
using kagome::crypto::BandersnatchProvider;
using kagome::crypto::BandersnatchProviderImpl;
using kagome::crypto::Bip39Provider;
using kagome::crypto::Bip39ProviderImpl;
using kagome::crypto::BoostRandomGenerator;
using kagome::crypto::EcdsaKeypair;
using kagome::crypto::EcdsaPrivateKey;
using kagome::crypto::EcdsaProvider;
using kagome::crypto::EcdsaProviderImpl;
using kagome::crypto::EcdsaPublicKey;
using kagome::crypto::Ed25519Keypair;
using kagome::crypto::Ed25519PrivateKey;
using kagome::crypto::Ed25519Provider;
using kagome::crypto::Ed25519ProviderImpl;
using kagome::crypto::Ed25519PublicKey;
using kagome::crypto::Ed25519Seed;
using kagome::crypto::HasherImpl;
using kagome::crypto::KeyStore;
using kagome::crypto::KeyStoreError;
using kagome::crypto::KeySuiteStoreImpl;
using kagome::crypto::KeyType;
using kagome::crypto::KeyTypes;
using kagome::crypto::Pbkdf2Provider;
using kagome::crypto::Pbkdf2ProviderImpl;
using kagome::crypto::SecureCleanGuard;
using kagome::crypto::Sr25519Keypair;
using kagome::crypto::Sr25519Provider;
using kagome::crypto::Sr25519ProviderImpl;
using kagome::crypto::Sr25519PublicKey;
using kagome::crypto::Sr25519SecretKey;
using kagome::crypto::Sr25519Seed;
using std::string_literals::operator""s;

static auto crypto_store_test_directory =
    kagome::filesystem::temp_directory_path() / "crypto_store_test";

struct KeyStoreTest : public test::BaseFS_Test {
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  KeyStoreTest()
      : BaseFS_Test(crypto_store_test_directory),
        ed_pair{
            .secret_key = Ed25519PrivateKey::fromHex(
                              SecureCleanGuard(
                                  "a4681403ba5b6a3f3bd0b0604ce439a78244c7d43b1"
                                  "27ec35cd8325602dd47fd"s))
                              .value(),
            .public_key = Ed25519PublicKey::fromHex(
                              "3e765f2bde3daadd443097b3145abf1f71f99f0aa946"
                              "960990fe02aa26b7fc72")
                              .value()} {}

  void SetUp() override {
    auto hasher = std::make_shared<HasherImpl>();
    auto csprng = std::make_shared<BoostRandomGenerator>();
    auto ecdsa_provider = std::make_shared<EcdsaProviderImpl>(hasher);
    auto ed25519_provider = std::make_shared<Ed25519ProviderImpl>(hasher);
    auto sr25519_provider = std::make_shared<Sr25519ProviderImpl>();
    auto bandersnatch_provider =
        std::make_shared<BandersnatchProviderImpl>(hasher);

    auto pbkdf2_provider = std::make_shared<Pbkdf2ProviderImpl>();
    bip39_provider =
        std::make_shared<Bip39ProviderImpl>(std::move(pbkdf2_provider), hasher);

    std::shared_ptr key_file_storage =
        kagome::crypto::KeyFileStorage::createAt(crypto_store_test_directory)
            .value();
    KeyStore::Config config{crypto_store_test_directory};
    key_store = std::make_shared<KeyStore>(
        std::make_unique<KeySuiteStoreImpl<Sr25519Provider>>(
            std::move(sr25519_provider),
            bip39_provider,
            csprng,
            key_file_storage),
        std::make_unique<KeySuiteStoreImpl<Ed25519Provider>>(
            ed25519_provider, bip39_provider, csprng, key_file_storage),
        std::make_unique<KeySuiteStoreImpl<EcdsaProvider>>(
            std::move(ecdsa_provider),
            bip39_provider,
            csprng,
            key_file_storage),
        std::make_unique<KeySuiteStoreImpl<BandersnatchProvider>>(
            std::move(bandersnatch_provider),
            bip39_provider,
            csprng,
            key_file_storage),
        ed25519_provider,
        std::make_shared<kagome::application::AppStateManagerMock>(),
        config);

    mnemonic =
        "ozone drill grab fiber curtain grace pudding thank cruise elder eight "
        "picnic";
    EXPECT_OUTCOME_TRUE(e, Buffer::fromHex("9e885d952ad362caeb4efe34a8e91bd2"));
    entropy = std::move(e);
    EXPECT_OUTCOME_TRUE(s,
                        Blob<32>::fromHex("a4681403ba5b6a3f3bd0b0604ce439a78244"
                                          "c7d43b127ec35cd8325602dd47fd"));
    seed = s;
    key_type = KeyTypes::BABE;

    EXPECT_OUTCOME_TRUE(
        sr_publ,
        Sr25519PublicKey::fromHex("56a03c8afc0e7a3a8b1d53bcc875ba5b6364754f9045"
                                  "16009b57ef3adf96f61f"));
    EXPECT_OUTCOME_TRUE(
        sr_secr,
        Sr25519SecretKey::fromHex(SecureCleanGuard{
            "ec96cb0816b67b045baae21841952a61ecb0612a109293e10c5453b950659c0a8b"
            "35b6d6196f33169334e36a05d624d9996d07243f9f71e638e3bc29a5330ec9"s}));
    sr_pair = {sr_secr, sr_publ};
  }

  bool isStoredOnDisk(KeyType kt, const Blob<32> &public_key) {
    auto file_name = kagome::crypto::encodeKeyFileName(kt, public_key);
    auto file_path = crypto_store_test_directory / file_name;
    return kagome::filesystem::exists(file_path);
  }

  std::shared_ptr<Bip39Provider> bip39_provider;
  std::shared_ptr<KeyStore> key_store;
  std::string mnemonic;
  Buffer entropy;
  Blob<32> seed;
  KeyType key_type;

  Ed25519Keypair ed_pair;
  Sr25519Keypair sr_pair;
};

/**
 * @given KeyStore instance, type, mnemonic and predefined key pair
 * @when generateEd25519Keypair is called
 * @then method call succeeds and result matches predefined key pair
 * @and generated key pair is stored in memory
 */
TEST_F(KeyStoreTest, generateEd25519KeypairMnemonicSuccess) {
  auto res = key_store->ed25519().findKeypair(key_type, ed_pair.public_key);
  ASSERT_EQ(res, std::nullopt);

  EXPECT_OUTCOME_TRUE(pair,
                      key_store->ed25519().generateKeypair(key_type, mnemonic));
  ASSERT_EQ(pair, ed_pair);

  // check that created pair is now contained in memory
  auto found = key_store->ed25519().findKeypair(key_type, pair.public_key);
  ASSERT_EQ(found, pair);

  // not stored on disk
  ASSERT_FALSE(isStoredOnDisk(key_type, pair.public_key));
}

/**
 * @given KeyStore instance, type, mnemonic and predefined key pair
 * @when generateSr25519Keypair is called
 * @then method call succeeds and result matches predefined key pair
 * @and generated key pair is stored in memory
 */
TEST_F(KeyStoreTest, generateSr25519KeypairMnemonicSuccess) {
  EXPECT_OUTCOME_TRUE(pair,
                      key_store->sr25519().generateKeypair(key_type, mnemonic));
  ASSERT_EQ(pair, sr_pair);

  // check that created pair is now contained in memory
  auto found = key_store->sr25519().findKeypair(key_type, pair.public_key);
  ASSERT_EQ(found, pair);
  // not stored on disk
  ASSERT_FALSE(isStoredOnDisk(key_type, pair.public_key));
}

/**
 * @given KeyStore instance, type, seed and predefined key pair
 * @when generateEd25519Keypair is called
 * @then method call succeeds and result matches predefined key pair
 * @and generated key pair is stored in memory
 */
TEST_F(KeyStoreTest, generateEd25519KeypairSeedSuccess) {
  auto res = key_store->ed25519().findKeypair(key_type, ed_pair.public_key);
  ASSERT_EQ(res, std::nullopt);

  EXPECT_OUTCOME_TRUE(pair,
                      key_store->ed25519().generateKeypair(
                          key_type, Ed25519Seed::from(SecureCleanGuard{seed})));
  ASSERT_EQ(pair, ed_pair);

  // check that created pair is now contained in memory
  auto found = key_store->ed25519().findKeypair(key_type, pair.public_key);
  ASSERT_EQ(found, pair);
  // not stored on disk
  ASSERT_FALSE(isStoredOnDisk(key_type, pair.public_key));
}

/**
 * @given KeyStore instance, type, seed and predefined key pair
 * @when generateSr25519Keypair is called
 * @then method call succeeds and result matches predefined key pair
 * @and key generated pair is stored in memory
 */
TEST_F(KeyStoreTest, generateSr25519KeypairSeedSuccess) {
  auto res = key_store->sr25519().findKeypair(key_type, sr_pair.public_key);
  ASSERT_EQ(res, std::nullopt);

  EXPECT_OUTCOME_TRUE(pair,
                      key_store->sr25519().generateKeypair(
                          key_type, Sr25519Seed::from(SecureCleanGuard{seed})));
  ASSERT_EQ(pair, sr_pair);

  // check that created pair is now contained in memory
  auto found = key_store->sr25519().findKeypair(key_type, pair.public_key);
  ASSERT_EQ(found, pair);

  // not stored on disk
  ASSERT_FALSE(isStoredOnDisk(key_type, pair.public_key));
}

/**
 * @given KeyStore instance, and key type
 * @when call generateEd25519KeypairOnDisk(key_type)
 * @then a new ed25519 key pair is generated and stored on disk
 */
TEST_F(KeyStoreTest, generateEd25519KeypairStoreSuccess) {
  EXPECT_OUTCOME_TRUE(pair,
                      key_store->ed25519().generateKeypairOnDisk(key_type));

  // check that created pair is contained in the storage on disk
  auto found = key_store->ed25519().findKeypair(key_type, pair.public_key);
  ASSERT_EQ(found, pair);

  // stored on disk
  ASSERT_TRUE(isStoredOnDisk(key_type, pair.public_key));
}

/**
 * @given KeyStore instance, and key type
 * @when call generateSr25519KeypairOnDisk(key_type)
 * @then a new ed25519 key pair is generated and stored on disk
 */
TEST_F(KeyStoreTest, generateSr25519KeypairStoreSuccess) {
  EXPECT_OUTCOME_TRUE(pair,
                      key_store->sr25519().generateKeypairOnDisk(key_type));

  // check that created pair is contained in the storage on disk
  auto found = key_store->sr25519().findKeypair(key_type, pair.public_key);
  ASSERT_EQ(found, pair);

  // stored on disk
  ASSERT_TRUE(isStoredOnDisk(key_type, pair.public_key));
}

/**
 * @given KeyStore instance, and key type
 * @when call getEd25519PublicKeys
 * @then collection of all ed25519 public keys of provided type is returned
 */
TEST_F(KeyStoreTest, getEd25519PublicKeysSuccess) {
  EXPECT_OUTCOME_TRUE(
      pair1, key_store->ed25519().generateKeypairOnDisk(KeyTypes::BABE));
  EXPECT_OUTCOME_TRUE(
      pair2, key_store->ed25519().generateKeypairOnDisk(KeyTypes::BABE));
  EXPECT_OUTCOME_SUCCESS(
      pair4, key_store->sr25519().generateKeypairOnDisk(KeyTypes::BABE));
  EXPECT_OUTCOME_SUCCESS(
      pair5, key_store->sr25519().generateKeypairOnDisk(KeyTypes::ACCOUNT));

  std::set<Ed25519PublicKey> ed_babe_keys_set = {pair1.public_key,
                                                 pair2.public_key};
  std::vector<Ed25519PublicKey> ed_babe_keys(ed_babe_keys_set.begin(),
                                             ed_babe_keys_set.end());
  auto keys = key_store->ed25519().getPublicKeys(KeyTypes::BABE).value();
  ASSERT_THAT(keys, testing::UnorderedElementsAreArray(ed_babe_keys));
}

/**
 * @given KeyStore instance, and key type
 * @when call getSr25519PublicKeys
 * @then collection of all sr25519 public keys of provided type is returned
 */
TEST_F(KeyStoreTest, getSr25519PublicKeysSuccess) {
  EXPECT_OUTCOME_TRUE(
      pair1, key_store->sr25519().generateKeypairOnDisk(KeyTypes::BABE));
  EXPECT_OUTCOME_TRUE(
      pair2, key_store->sr25519().generateKeypairOnDisk(KeyTypes::BABE));
  EXPECT_OUTCOME_SUCCESS(
      pair4, key_store->ed25519().generateKeypairOnDisk(KeyTypes::BABE));
  EXPECT_OUTCOME_SUCCESS(
      pair5, key_store->ed25519().generateKeypairOnDisk(KeyTypes::ACCOUNT));

  std::set<Sr25519PublicKey> sr_babe_keys_set = {pair1.public_key,
                                                 pair2.public_key};
  std::vector<Sr25519PublicKey> sr_babe_keys(sr_babe_keys_set.begin(),
                                             sr_babe_keys_set.end());

  auto keys = key_store->sr25519().getPublicKeys(KeyTypes::BABE).value();
  ASSERT_THAT(keys, testing::UnorderedElementsAreArray(sr_babe_keys));
}
