/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "host_api/impl/crypto_extension.hpp"

#include <algorithm>

#include <gtest/gtest.h>
#include <span>

#include <qtils/test/outcome.hpp>
#include "crypto/ecdsa/ecdsa_provider_impl.hpp"
#include "crypto/ed25519/ed25519_provider_impl.hpp"
#include "crypto/hasher/hasher_impl.hpp"
#include "crypto/key_store/key_store_impl.hpp"
#include "crypto/random_generator/boost_generator.hpp"
#include "crypto/secp256k1/secp256k1_provider_impl.hpp"
#include "crypto/sr25519/sr25519_provider_impl.hpp"
#include "mock/core/crypto/key_store_mock.hpp"
#include "mock/core/runtime/memory_provider_mock.hpp"
#include "scale/kagome_scale.hpp"
#include "testutil/literals.hpp"
#include "testutil/prepare_loggers.hpp"
#include "testutil/runtime/memory.hpp"

using namespace kagome::host_api;
using kagome::common::Blob;
using kagome::common::Buffer;
using kagome::common::BufferView;
using kagome::crypto::BoostRandomGenerator;
using kagome::crypto::CSPRNG;
using kagome::crypto::EcdsaKeypair;
using kagome::crypto::EcdsaPrivateKey;
using kagome::crypto::EcdsaProvider;
using kagome::crypto::EcdsaProviderImpl;
using kagome::crypto::EcdsaPublicKey;
using kagome::crypto::EcdsaSignature;
using kagome::crypto::Ed25519Keypair;
using kagome::crypto::Ed25519PrivateKey;
using kagome::crypto::Ed25519Provider;
using kagome::crypto::Ed25519ProviderImpl;
using kagome::crypto::Ed25519PublicKey;
using kagome::crypto::Ed25519Seed;
using kagome::crypto::Ed25519Signature;
using kagome::crypto::Hasher;
using kagome::crypto::HasherImpl;
using kagome::crypto::KeyStore;
using kagome::crypto::KeyStoreMock;
using kagome::crypto::KeySuiteStoreMock;
using kagome::crypto::KeyType;
using kagome::crypto::KeyTypes;
using kagome::crypto::Secp256k1Provider;
using kagome::crypto::Secp256k1ProviderImpl;
using kagome::crypto::SecureBuffer;
using kagome::crypto::SecureCleanGuard;
using kagome::crypto::Sr25519Keypair;
using kagome::crypto::Sr25519Provider;
using kagome::crypto::Sr25519ProviderImpl;
using kagome::crypto::Sr25519PublicKey;
using kagome::crypto::Sr25519SecretKey;
using kagome::crypto::Sr25519Seed;
using kagome::crypto::Sr25519Signature;
using kagome::crypto::secp256k1::Secp256k1VerifyError;
using kagome::runtime::MemoryProviderMock;
using kagome::runtime::TestMemory;
using kagome::runtime::WasmPointer;
using kagome::scale::encode;

using ::testing::Return;

namespace ecdsa_constants = kagome::crypto::constants::ecdsa;
namespace sr25519_constants = kagome::crypto::constants::sr25519;
namespace ed25519_constants = kagome::crypto::constants::ed25519;
namespace secp256k1 = kagome::crypto::secp256k1;

class CryptoExtensionTest : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  using RecoverUncompressedPublicKeyReturnValue =
      boost::variant<secp256k1::PublicKey, Secp256k1VerifyError>;
  using RecoverCompressedPublicKeyReturnValue =
      boost::variant<secp256k1::CompressedPublicKey, Secp256k1VerifyError>;

  void SetUp() override {
    memory_provider_ = std::make_shared<MemoryProviderMock>();
    EXPECT_CALL(*memory_provider_, getCurrentMemory())
        .WillRepeatedly(Return(std::ref(memory_.memory)));

    random_generator_ = std::make_shared<BoostRandomGenerator>();
    hasher_ = std::make_shared<HasherImpl>();
    ecdsa_provider_ = std::make_shared<EcdsaProviderImpl>(hasher_);
    sr25519_provider_ = std::make_shared<Sr25519ProviderImpl>();
    ed25519_provider_ = std::make_shared<Ed25519ProviderImpl>(hasher_);
    secp256k1_provider_ = std::make_shared<Secp256k1ProviderImpl>();

    key_store_ = std::make_shared<KeyStoreMock>();
    crypto_ext_ = std::make_shared<CryptoExtension>(memory_provider_,
                                                    sr25519_provider_,
                                                    ecdsa_provider_,
                                                    ed25519_provider_,
                                                    secp256k1_provider_,
                                                    hasher_,
                                                    key_store_);

    ASSERT_OUTCOME_SUCCESS(
        seed_tmp, kagome::common::Blob<32>::fromHexWithPrefix(seed_hex));
    std::copy_n(seed_tmp.begin(), Blob<32>::size(), seed.begin());

    // scale-encoded string
    std::optional<std::span<uint8_t>> optional_seed(seed);
    seed_buffer.put(encode(optional_seed).value());
    std::optional<std::string> optional_mnemonic(mnemonic);
    mnemonic_buffer.put(encode(optional_mnemonic).value());

    sr25519_keypair =
        sr25519_provider_
            ->generateKeypair(Sr25519Seed::from(SecureCleanGuard{seed}), {})
            .value();
    sr25519_signature = sr25519_provider_->sign(sr25519_keypair, input).value();

    ed25519_keypair =
        ed25519_provider_
            ->generateKeypair(Ed25519Seed::from(SecureCleanGuard{seed}), {})
            .value();
    ed25519_signature = ed25519_provider_->sign(ed25519_keypair, input).value();

    secp_message_hash =
        secp256k1::MessageHash::fromSpan(secp_message_vector).value();
    secp_uncompressed_public_key =
        secp256k1::UncompressedPublicKey::fromSpan(secp_public_key_bytes)
            .value();
    secp_compressed_pyblic_key = secp256k1::CompressedPublicKey::fromSpan(
                                     secp_public_key_compressed_bytes)
                                     .value();
    // first byte contains 0x04
    // and needs to be omitted in runtime api return value
    secp_truncated_public_key = secp256k1::PublicKey::fromSpan(
                                    std::span(secp_public_key_bytes).subspan(1))
                                    .value();
    secp_signature =
        secp256k1::RSVSignature::fromSpan(secp_signature_bytes).value();

    scale_encoded_secp_truncated_public_key =
        Buffer(encode(RecoverUncompressedPublicKeyReturnValue(
                          secp_truncated_public_key))
                   .value());

    scale_encoded_secp_compressed_public_key =
        Buffer(encode(RecoverCompressedPublicKeyReturnValue(
                          secp_compressed_pyblic_key))
                   .value());

    // this value suits both compressed & uncompressed failure tests
    secp_invalid_signature_error =
        Buffer(encode(RecoverCompressedPublicKeyReturnValue(
                          kagome::crypto::secp256k1::secp256k1_verify_error::
                              kInvalidSignature))
                   .value());

    ed_public_keys_result
        .putUint8(4)  // scale-encoded size // 1
        .put(ed25519_keypair.public_key);

    sr_public_keys_result
        .putUint8(4)  // scale-encoded size // 1
        .put(sr25519_keypair.public_key);

    ed_public_keys.emplace_back(ed25519_keypair.public_key);
    sr_public_keys.emplace_back(sr25519_keypair.public_key);
    ed_public_key_buffer.put(ed25519_keypair.public_key);
    sr_public_key_buffer.put(sr25519_keypair.public_key);

    ed25519_signature_result      // encoded optional value
        .putUint8(1)              // 1 means that value presents
        .put(ed25519_signature);  // the value itself

    sr25519_signature_result      // encoded optional value
        .putUint8(1)              // 1 means that value presents
        .put(sr25519_signature);  // the value itself

    // the return value is scale-encoded 'boost::option's none',
    // so it's "0" for all types
    ed_sr_signature_failure_result_buffer.putUint8(0);
  }

  void bytesN(WasmPointer ptr, BufferView expected) {
    EXPECT_EQ(memory_.memory.view(ptr, expected.size()).value(),
              SpanAdl{expected});
  }

 protected:
  TestMemory memory_;
  std::shared_ptr<MemoryProviderMock> memory_provider_;
  std::shared_ptr<CSPRNG> random_generator_;
  std::shared_ptr<EcdsaProvider> ecdsa_provider_;
  std::shared_ptr<Sr25519Provider> sr25519_provider_;
  std::shared_ptr<Ed25519Provider> ed25519_provider_;
  std::shared_ptr<Secp256k1Provider> secp256k1_provider_;
  std::shared_ptr<Hasher> hasher_;
  std::shared_ptr<KeyStoreMock> key_store_;
  std::shared_ptr<CryptoExtension> crypto_ext_;

  KeyType key_type = KeyTypes::BABE;
  inline static Buffer input{"6920616d2064617461"_unhex};

  Sr25519Signature sr25519_signature{};
  Sr25519Keypair sr25519_keypair{};
  Ed25519Signature ed25519_signature{};
  Ed25519Keypair ed25519_keypair{};

  inline static Buffer blake2b_128_result{
      "de944c5c12e55ee9a07cf5bf4b674995"_unhex};

  inline static Buffer blake2b_256_result{
      "ba67336efd6a3df3a70eeb757860763036785c182ff4cf587541a0068d09f5b2"_unhex};

  inline static Buffer keccak_result{
      "65aac3ad8b88cb79396da4c8b6a8cb6b5b74b0f6534a3e4e5e8ad68658feccf4"_unhex};

  inline static Buffer sha2_256_result{
      "3dabee24d43ded7266178f585eea5c1a6f2c18b316a6f5e946e137f9ef9b5f69"_unhex};

  inline static Buffer twox_input{"414243444546"_unhex};

  inline static Buffer twox64_result{184, 65, 176, 250, 243, 129, 181, 3};

  inline static Buffer twox128_result{
      184, 65, 176, 250, 243, 129, 181, 3, 77, 82, 63, 150, 129, 221, 191, 251};

  inline static Buffer twox256_result{184, 65,  176, 250, 243, 129, 181, 3,
                                      77,  82,  63,  150, 129, 221, 191, 251,
                                      33,  226, 149, 136, 6,   232, 81,  118,
                                      200, 28,  69,  219, 120, 179, 208, 237};

  inline static Buffer secp_public_key_bytes{
      "04f821bc128a43d9b0516969111e19a40bab417f45181d692d0519a3b35573cb63178403d12eb41d7702913a70ebc1c64438002a1474e1328276b7dcdacb511fc3"_hex2buf};
  inline static Buffer secp_public_key_compressed_bytes{
      "03f821bc128a43d9b0516969111e19a40bab417f45181d692d0519a3b35573cb63"_hex2buf};
  inline static Buffer secp_signature_bytes{
      "ebdedee38bcf530f13c1b5c8717d974a6f8bd25a7e3707ca36c7ee7efd5aa6c557bcc67906975696cbb28a556b649e5fbf5ce51831572cd54add248c4d023fcf01"_hex2buf};
  inline static Buffer secp_message_vector{
      "e13d3f3f21115294edf249cfdcb262a4f96d86943b63426c7635b6d94a5434c7"_hex2buf};
  secp256k1::MessageHash secp_message_hash;
  Buffer secp_invalid_signature_error;
  Buffer ed_public_keys_result;
  Buffer sr_public_keys_result;
  Buffer ed_public_key_buffer;
  Buffer sr_public_key_buffer;
  Buffer ed25519_signature_result;
  Buffer sr25519_signature_result;

  std::vector<Ed25519PublicKey> ed_public_keys;
  std::vector<Sr25519PublicKey> sr_public_keys;

  secp256k1::RSVSignature secp_signature;  ///< secp256k1 RSV-signature
  secp256k1::UncompressedPublicKey
      secp_uncompressed_public_key;  ///< secp256k1 uncompressed public key
  secp256k1::CompressedPublicKey
      secp_compressed_pyblic_key;  ///< secp256k1 compressed public key
  secp256k1::PublicKey secp_truncated_public_key;  ///< secp256k1 truncated
                                                   ///< uncompressed public key

  Buffer scale_encoded_secp_truncated_public_key;
  Buffer scale_encoded_secp_compressed_public_key;
  Buffer ed_sr_signature_failure_result_buffer;

  Blob<32> seed;  ///< seed is for generating sr25519 and ed25519 keys
  inline static std::string seed_hex =
      "0xa4681403ba5b6a3f3bd0b0604ce439a78244c7d43b127ec35cd8325602dd47fd";
  Buffer seed_buffer;
  inline static std::string mnemonic =
      "ozone drill grab fiber curtain grace pudding thank cruise elder eight "
      "picnic";
  Buffer mnemonic_buffer;
};

/**
 * @given initialized crypto extension @and data, which can be
 * blake2b_128-hashed
 * @when hashing that data
 * @then resulting hash is correct
 */
TEST_F(CryptoExtensionTest, Blake2_128Valid) {
  bytesN(crypto_ext_->ext_hashing_blake2_128_version_1(memory_[input]),
         blake2b_128_result);
}

/**
 * @given initialized crypto extension @and data, which can be
 * blake2b_256-hashed
 * @when hashing that data
 * @then resulting hash is correct
 */
TEST_F(CryptoExtensionTest, Blake2_256Valid) {
  bytesN(crypto_ext_->ext_hashing_blake2_256_version_1(memory_[input]),
         blake2b_256_result);
}

/**
 * @given initialized crypto extension @and data, which can be keccak-hashed
 * @when hashing that data
 * @then resulting hash is correct
 */
TEST_F(CryptoExtensionTest, KeccakValid) {
  bytesN(crypto_ext_->ext_hashing_keccak_256_version_1(memory_[input]),
         keccak_result);
}

/**
 * @given initialized crypto extension @and ed25519-signed message
 * @when verifying signature of this message
 * @then verification is successful
 */
TEST_F(CryptoExtensionTest, Ed25519VerifySuccess) {
  SecureBuffer<> seed_buf(Ed25519Seed::size());
  random_generator_->fillRandomly(seed_buf);
  auto seed = Ed25519Seed::from(std::move(seed_buf)).value();
  auto keypair = ed25519_provider_->generateKeypair(seed, {}).value();
  ASSERT_OUTCOME_SUCCESS(signature, ed25519_provider_->sign(keypair, input));

  ASSERT_EQ(
      crypto_ext_->ext_crypto_ed25519_verify_version_1(
          memory_[signature], memory_[input], memory_[keypair.public_key]),
      CryptoExtension::kVerifySuccess);
}

/**
 * @given initialized crypto extension @and incorrect ed25519 signature for
 some
 * message
 * @when verifying signature of this message
 * @then verification fails
 */
TEST_F(CryptoExtensionTest, Ed25519VerifyFailure) {
  SecureBuffer<> seed_buf(Ed25519Seed::size());
  random_generator_->fillRandomly(seed_buf);
  auto seed = Ed25519Seed::from(std::move(seed_buf)).value();

  auto keypair = ed25519_provider_->generateKeypair(seed, {}).value();
  Ed25519Signature invalid_signature;
  invalid_signature.fill(0x11);

  ASSERT_EQ(crypto_ext_->ext_crypto_ed25519_verify_version_1(
                memory_[invalid_signature],
                memory_[input],
                memory_[keypair.public_key]),
            CryptoExtension::kVerifyFail);
}

/**
 * @given initialized crypto extension @and sr25519-signed message
 * @when verifying signature of this message
 * @then verification is successful
 */
TEST_F(CryptoExtensionTest, Sr25519VerifySuccess) {
  ASSERT_EQ(crypto_ext_->ext_crypto_sr25519_verify_version_2(
                memory_[sr25519_signature],
                memory_[input],
                memory_[sr25519_keypair.public_key]),
            CryptoExtension::kVerifySuccess);
}

/**
 * @given initialized crypto extension @and sr25519-signed message
 * @when verifying signature of this message
 * @then verification fails
 */
TEST_F(CryptoExtensionTest, Sr25519VerifyFailure) {
  auto false_signature = sr25519_signature;
  ++false_signature[0];
  ++false_signature[1];
  ++false_signature[2];
  ++false_signature[3];

  ASSERT_EQ(crypto_ext_->ext_crypto_sr25519_verify_version_2(
                memory_[false_signature],
                memory_[input],
                memory_[sr25519_keypair.public_key]),
            CryptoExtension::kVerifyFail);
}

/**
 * @given initialized crypto extensions @and secp256k1 signature and message
 * @when call recovery public secp256k1 uncompressed key
 * @then resulting public key is correct
 */
TEST_F(CryptoExtensionTest, Secp256k1RecoverUncompressedSuccess) {
  EXPECT_EQ(memory_[crypto_ext_->ext_crypto_secp256k1_ecdsa_recover_version_1(
                memory_[secp_signature], memory_[secp_message_hash])],
            scale_encoded_secp_truncated_public_key);
}

/**
 * @given initialized crypto extensions
 * @and a damaged secp256k1 signature and message
 * @when call recovery public secp256k1 uncompressed key
 * @then error code is returned
 */
TEST_F(CryptoExtensionTest, Secp256k1RecoverUncompressedFailure) {
  auto sig_buffer = secp_signature;
  // corrupt signature
  std::fill(sig_buffer.begin() + 2, sig_buffer.begin() + 10, 0xFF);

  ASSERT_EQ(memory_[crypto_ext_->ext_crypto_secp256k1_ecdsa_recover_version_1(
                memory_[sig_buffer], memory_[secp_message_hash])],
            secp_invalid_signature_error);
}

/**
 * @given initialized crypto extensions @and secp256k1 signature and message
 * @when call recovery public secp256k1 compressed key
 * @then resulting public key is correct
 */
TEST_F(CryptoExtensionTest, Secp256k1RecoverCompressed) {
  ASSERT_EQ(
      memory_[crypto_ext_
                  ->ext_crypto_secp256k1_ecdsa_recover_compressed_version_1(
                      memory_[secp_signature], memory_[secp_message_hash])],
      scale_encoded_secp_compressed_public_key);
}

/**
 * @given initialized crypto extensions
 * @and a damaged secp256k1 signature and message
 * @when call recovery public secp256k1 compressed key
 * @then error code is returned
 */
TEST_F(CryptoExtensionTest, Secp256k1RecoverCompressedFailure) {
  auto sig_buffer = secp_signature;
  // corrupt signature
  std::fill(sig_buffer.begin() + 2, sig_buffer.begin() + 10, 0xFF);

  ASSERT_EQ(
      memory_[crypto_ext_
                  ->ext_crypto_secp256k1_ecdsa_recover_compressed_version_1(
                      memory_[sig_buffer], memory_[secp_message_hash])],
      secp_invalid_signature_error);
}

/**
 * @given initialized crypto extension, key type
 * @when call ext_ed25519_public_keys_v1 of crypto extension
 * @then we get serialized set of existing ed25519 keys
 */
TEST_F(CryptoExtensionTest, Ed25519GetPublicKeysSuccess) {
  EXPECT_CALL(key_store_->ed25519(), getPublicKeys(key_type))
      .WillOnce(Return(ed_public_keys));

  ASSERT_EQ(memory_[crypto_ext_->ext_crypto_ed25519_public_keys_version_1(
                memory_.store32u(key_type))],
            ed_public_keys_result);
}

/**
 * @given initialized crypto extension, key type
 * @when call ext_sr25519_public_keys_v1 of crypto extension
 * @then we get serialized set of existing sr25519 keys
 */
TEST_F(CryptoExtensionTest, Sr25519GetPublicKeysSuccess) {
  EXPECT_CALL(key_store_->sr25519(), getPublicKeys(key_type))
      .WillOnce(Return(sr_public_keys));

  ASSERT_EQ(memory_[crypto_ext_->ext_crypto_sr25519_public_keys_version_1(
                memory_.store32u(key_type))],
            sr_public_keys_result);
}

/**
 * @given initialized crypto extension, key type, public key value and message
 * @when call ext_ed25519_sign_v1 of crypto extension
 * @then we get a valid signature
 */
TEST_F(CryptoExtensionTest, Ed25519SignSuccess) {
  EXPECT_CALL(key_store_->ed25519(),
              findKeypair(key_type, ed25519_keypair.public_key))
      .WillOnce(Return(ed25519_keypair));

  ASSERT_EQ(memory_[crypto_ext_->ext_crypto_ed25519_sign_version_1(
                memory_.store32u(key_type),
                memory_[ed25519_keypair.public_key],
                memory_[input])],
            ed25519_signature_result);
}

/**
 * @given initialized crypto extension,
 * key type, not existing public key and message
 * @when call ext_ed25519_sign_v1 of crypto extension
 * @then we get a valid serialized error
 */
TEST_F(CryptoExtensionTest, Ed25519SignFailure) {
  EXPECT_CALL(key_store_->ed25519(),
              findKeypair(key_type, ed25519_keypair.public_key))
      .WillOnce(Return(std::nullopt));

  ASSERT_EQ(memory_[crypto_ext_->ext_crypto_ed25519_sign_version_1(
                memory_.store32u(key_type),
                memory_[ed25519_keypair.public_key],
                memory_[input])],
            ed_sr_signature_failure_result_buffer);
}

/**
 * @given initialized crypto extension, key type, public key value and message
 * @when call ext_sr25519_sign_v1 of crypto extension
 * @then we get a valid signature
 */
TEST_F(CryptoExtensionTest, Sr25519SignSuccess) {
  EXPECT_CALL(key_store_->sr25519(),
              findKeypair(key_type, sr25519_keypair.public_key))
      .WillOnce(Return(sr25519_keypair));

  auto sig = memory_
                 .decode<std::optional<Sr25519Signature>>(
                     crypto_ext_->ext_crypto_sr25519_sign_version_1(
                         memory_.store32u(key_type),
                         memory_[sr25519_keypair.public_key],
                         memory_[input]))
                 .value();
  ASSERT_TRUE(sr25519_provider_->verify(sig, input, sr25519_keypair.public_key)
                  .value());
}

/**
 * @given initialized crypto extension,
 * key type, not existing public key and message
 * @when call ext_sr25519_sign_v1 of crypto extension
 * @then we get a valid serialized error
 */
TEST_F(CryptoExtensionTest, Sr25519SignFailure) {
  EXPECT_CALL(key_store_->sr25519(),
              findKeypair(key_type, sr25519_keypair.public_key))
      .WillOnce(Return(std::nullopt));

  ASSERT_EQ(memory_[crypto_ext_->ext_crypto_sr25519_sign_version_1(
                memory_.store32u(key_type),
                memory_[sr25519_keypair.public_key],
                memory_[input])],
            ed_sr_signature_failure_result_buffer);
}

/**
 * @given initialized crypto extension, key type and hexified seed
 * @when call generate ed25519 keypair method of crypto extension
 * @then a new ed25519 keypair is successfully generated and stored
 */
TEST_F(CryptoExtensionTest, Ed25519GenerateByHexSeedSuccess) {
  EXPECT_CALL(key_store_->ed25519(),
              generateKeypair(key_type, std::string_view(mnemonic)))
      .WillOnce(Return(ed25519_keypair));
  bytesN(crypto_ext_->ext_crypto_ed25519_generate_version_1(
             memory_.store32u(key_type), memory_[mnemonic_buffer]),
         ed_public_key_buffer);
}

/**
 * @given initialized crypto extension, key type and mnemonic phrase seed
 * @when call generate ed25519 keypair method of crypto extension
 * @then a new ed25519 keypair is successfully generated and stored
 */
TEST_F(CryptoExtensionTest, Ed25519GenerateByMnemonicSuccess) {
  EXPECT_CALL(key_store_->ed25519(),
              generateKeypair(key_type, std::string_view(mnemonic)))
      .WillOnce(Return(ed25519_keypair));
  bytesN(crypto_ext_->ext_crypto_ed25519_generate_version_1(
             memory_.store32u(key_type), memory_[mnemonic_buffer]),
         ed_public_key_buffer);
}

/**
 * @given initialized crypto extension, key type and hexified seed
 * @when call generate sr25519 keypair method of crypto extension
 * @then a new sr25519 keypair is successfully generated and stored
 */
TEST_F(CryptoExtensionTest, Sr25519GenerateByHexSeedSuccess) {
  EXPECT_CALL(key_store_->sr25519(),
              generateKeypair(key_type, std::string_view(mnemonic)))
      .WillOnce(Return(sr25519_keypair));
  bytesN(crypto_ext_->ext_crypto_sr25519_generate_version_1(
             memory_.store32u(key_type), memory_[mnemonic_buffer]),
         sr_public_key_buffer);
}

/**
 * @given initialized crypto extension, key type and mnemonic phrase seed
 * @when call generate sr25519 keypair method of crypto extension
 * @then a new sr25519 keypair is successfully generated and stored
 */
TEST_F(CryptoExtensionTest, Sr25519GenerateByMnemonicSuccess) {
  EXPECT_CALL(key_store_->sr25519(),
              generateKeypair(key_type, std::string_view(mnemonic)))
      .WillOnce(Return(sr25519_keypair));
  bytesN(crypto_ext_->ext_crypto_sr25519_generate_version_1(
             memory_.store32u(key_type), memory_[mnemonic_buffer]),
         sr_public_key_buffer);
}

/**
 * @given initialized crypto extension @and data, which can be sha2_256-hashed
 * @when hashing that data
 * @then resulting hash is correct
 */
TEST_F(CryptoExtensionTest, Sha2_256Version1_Success) {
  bytesN(crypto_ext_->ext_hashing_sha2_256_version_1(memory_[input]),
         sha2_256_result);
}

/**
 * @given initialized crypto extension @and data, which can be twox_256-hashed
 * @when hashing that data
 * @then resulting hash is correct
 */
TEST_F(CryptoExtensionTest, Twox_256Version1_Success) {
  bytesN(crypto_ext_->ext_hashing_twox_256_version_1(memory_[twox_input]),
         twox256_result);
}

/**
 * @given initialized crypto extension @and data, which can be twox_128-hashed
 * @when hashing that data
 * @then resulting hash is correct
 */
TEST_F(CryptoExtensionTest, Twox_128Version1_Success) {
  bytesN(crypto_ext_->ext_hashing_twox_128_version_1(memory_[twox_input]),
         twox128_result);
}

/**
 * @given initialized crypto extension @and data, which can be twox_128-hashed
 * @when hashing that data
 * @then resulting hash is correct
 */
TEST_F(CryptoExtensionTest, Twox_64Version1_Success) {
  bytesN(crypto_ext_->ext_hashing_twox_64_version_1(memory_[twox_input]),
         twox64_result);
}
