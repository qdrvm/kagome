/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "host_api/impl/crypto_extension.hpp"

#include <algorithm>

#include <gtest/gtest.h>
#include <gsl/span>

#include "crypto/bip39/impl/bip39_provider_impl.hpp"
#include "crypto/crypto_store/crypto_store_impl.hpp"
#include "crypto/ed25519/ed25519_provider_impl.hpp"
#include "crypto/hasher/hasher_impl.hpp"
#include "crypto/pbkdf2/impl/pbkdf2_provider_impl.hpp"
#include "crypto/random_generator/boost_generator.hpp"
#include "crypto/secp256k1/secp256k1_provider_impl.hpp"
#include "crypto/sr25519/sr25519_provider_impl.hpp"
#include "mock/core/crypto/crypto_store_mock.hpp"
#include "mock/core/runtime/memory_mock.hpp"
#include "mock/core/runtime/memory_provider_mock.hpp"
#include "runtime/ptr_size.hpp"
#include "scale/scale.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/prepare_loggers.hpp"

using namespace kagome::host_api;
using kagome::common::Blob;
using kagome::common::Buffer;
using kagome::crypto::Bip39Provider;
using kagome::crypto::Bip39ProviderImpl;
using kagome::crypto::BoostRandomGenerator;
using kagome::crypto::CryptoStore;
using kagome::crypto::CryptoStoreImpl;
using kagome::crypto::CryptoStoreMock;
using kagome::crypto::CSPRNG;
using kagome::crypto::Ed25519Keypair;
using kagome::crypto::Ed25519PrivateKey;
using kagome::crypto::Ed25519Provider;
using kagome::crypto::Ed25519ProviderImpl;
using kagome::crypto::Ed25519PublicKey;
using kagome::crypto::Ed25519Signature;
using kagome::crypto::Hasher;
using kagome::crypto::HasherImpl;
using kagome::crypto::Pbkdf2Provider;
using kagome::crypto::Pbkdf2ProviderImpl;
using kagome::crypto::Secp256k1Provider;
using kagome::crypto::Secp256k1ProviderImpl;
using kagome::crypto::Sr25519Keypair;
using kagome::crypto::Sr25519Provider;
using kagome::crypto::Sr25519ProviderImpl;
using kagome::crypto::Sr25519PublicKey;
using kagome::crypto::Sr25519SecretKey;
using kagome::crypto::Sr25519Signature;
using kagome::crypto::secp256k1::EcdsaVerifyError;
using kagome::runtime::MemoryMock;
using kagome::runtime::MemoryProviderMock;
using kagome::runtime::Memory;
using kagome::runtime::WasmPointer;
using kagome::runtime::PtrSize;
using kagome::runtime::WasmSize;
using kagome::runtime::WasmSpan;

using ::testing::Return;

namespace sr25519_constants = kagome::crypto::constants::sr25519;
namespace ed25519_constants = kagome::crypto::constants::ed25519;
namespace ecdsa = kagome::crypto::secp256k1;

// The point is that sr25519 signature can have many valid values
// so we can only verify whether it is correct
MATCHER_P3(VerifySr25519Signature,
           provider,
           msg,
           pubkey,
           "check if matched sr25519 signature is correct") {
  Sr25519Signature signature{};
  std::copy_n(arg.begin(), Sr25519Signature::size(), signature.begin());

  return static_cast<bool>(provider->verify(signature, msg, pubkey));
}

class CryptoExtensionTest : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  using RecoverUncompressedPublicKeyReturnValue =
      boost::variant<ecdsa::PublicKey, EcdsaVerifyError>;
  using RecoverCompressedPublicKeyReturnValue =
      boost::variant<ecdsa::CompressedPublicKey, EcdsaVerifyError>;

  void SetUp() override {
    memory_ = std::make_shared<MemoryMock>();
    memory_provider_ = std::make_shared<MemoryProviderMock>();
    EXPECT_CALL(*memory_provider_, getCurrentMemory())
        .WillRepeatedly(Return(boost::optional<Memory&>(*memory_)));

    random_generator_ = std::make_shared<BoostRandomGenerator>();
    sr25519_provider_ =
        std::make_shared<Sr25519ProviderImpl>(random_generator_);
    ed25519_provider_ =
        std::make_shared<Ed25519ProviderImpl>(random_generator_);
    secp256k1_provider_ = std::make_shared<Secp256k1ProviderImpl>();
    hasher_ = std::make_shared<HasherImpl>();
    bip39_provider_ = std::make_shared<Bip39ProviderImpl>(
        std::make_shared<Pbkdf2ProviderImpl>());

    crypto_store_ = std::make_shared<CryptoStoreMock>();
    crypto_ext_ = std::make_shared<CryptoExtension>(memory_provider_,
                                                    sr25519_provider_,
                                                    ed25519_provider_,
                                                    secp256k1_provider_,
                                                    hasher_,
                                                    crypto_store_,
                                                    bip39_provider_);

    EXPECT_OUTCOME_TRUE(seed_tmp,
                        kagome::common::Blob<32>::fromHexWithPrefix(seed_hex));
    std::copy_n(seed_tmp.begin(), Blob<32>::size(), seed.begin());

    // scale-encoded string
    boost::optional<gsl::span<uint8_t>> optional_seed(seed);
    seed_buffer.put(kagome::scale::encode(optional_seed).value());
    boost::optional<std::string> optional_mnemonic(mnemonic);
    mnemonic_buffer.put(kagome::scale::encode(optional_mnemonic).value());

    sr25519_keypair = sr25519_provider_->generateKeypair(seed);
    sr25519_signature = sr25519_provider_->sign(sr25519_keypair, input).value();

    ed25519_keypair = ed25519_provider_->generateKeypair(seed);
    ed25519_signature = ed25519_provider_->sign(ed25519_keypair, input).value();

    secp_message_hash =
        ecdsa::MessageHash::fromSpan(secp_message_vector).value();
    secp_uncompressed_public_key =
        ecdsa::UncompressedPublicKey::fromSpan(secp_public_key_bytes).value();
    secp_compressed_pyblic_key =
        ecdsa::CompressedPublicKey::fromSpan(secp_public_key_compressed_bytes)
            .value();
    // first byte contains 0x04
    // and needs to be omitted in runtime api return value
    secp_truncated_public_key =
        ecdsa::PublicKey::fromSpan(
            gsl::make_span(secp_public_key_bytes).subspan(1))
            .value();
    secp_signature =
        ecdsa::RSVSignature::fromSpan(secp_signature_bytes).value();

    scale_encoded_secp_truncated_public_key =
        Buffer(kagome::scale::encode(RecoverUncompressedPublicKeyReturnValue(
                                         secp_truncated_public_key))
                   .value());

    scale_encoded_secp_compressed_public_key =
        Buffer(kagome::scale::encode(RecoverCompressedPublicKeyReturnValue(
                                         secp_compressed_pyblic_key))
                   .value());

    // this value suits both compressed & uncompressed failure tests
    secp_invalid_signature_error = Buffer(
        kagome::scale::encode(RecoverCompressedPublicKeyReturnValue(
                                  kagome::crypto::secp256k1::
                                      ecdsa_verify_error::kInvalidSignature))
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

 protected:
  std::shared_ptr<MemoryMock> memory_;
  std::shared_ptr<MemoryProviderMock> memory_provider_;
  std::shared_ptr<CSPRNG> random_generator_;
  std::shared_ptr<Sr25519Provider> sr25519_provider_;
  std::shared_ptr<Ed25519Provider> ed25519_provider_;
  std::shared_ptr<Secp256k1Provider> secp256k1_provider_;
  std::shared_ptr<Hasher> hasher_;
  std::shared_ptr<CryptoStoreMock> crypto_store_;
  std::shared_ptr<CryptoExtension> crypto_ext_;
  std::shared_ptr<Bip39Provider> bip39_provider_;

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
  ecdsa::MessageHash secp_message_hash;
  Buffer secp_invalid_signature_error;
  Buffer ed_public_keys_result;
  Buffer sr_public_keys_result;
  Buffer ed_public_key_buffer;
  Buffer sr_public_key_buffer;
  Buffer ed25519_signature_result;
  Buffer sr25519_signature_result;

  std::vector<Ed25519PublicKey> ed_public_keys;
  std::vector<Sr25519PublicKey> sr_public_keys;

  ecdsa::RSVSignature secp_signature;  ///< secp256k1 RSV-signature
  ecdsa::UncompressedPublicKey
      secp_uncompressed_public_key;  ///< secp256k1 uncompressed public key
  ecdsa::CompressedPublicKey
      secp_compressed_pyblic_key;  ///< secp256k1 compressed public key
  ecdsa::PublicKey secp_truncated_public_key;  ///< secp256k1 truncated
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
  WasmPointer data = 0;
  WasmSize size = input.size();
  WasmPointer out_ptr = 42;

  EXPECT_CALL(*memory_, loadN(data, size)).WillOnce(Return(input));
  EXPECT_CALL(
      *memory_,
      storeBuffer(gsl::span<const uint8_t>(blake2b_128_result)))
      .WillOnce(Return(out_ptr));

  ASSERT_EQ(out_ptr,
            crypto_ext_->ext_hashing_blake2_128_version_1(
                PtrSize{data, size}.combine()));
}

/**
 * @given initialized crypto extension @and data, which can be
 * blake2b_256-hashed
 * @when hashing that data
 * @then resulting hash is correct
 */
TEST_F(CryptoExtensionTest, Blake2_256Valid) {
  WasmPointer data = 0;
  WasmSize size = input.size();
  WasmPointer out_ptr = 42;

  EXPECT_CALL(*memory_, loadN(data, size)).WillOnce(Return(input));
  EXPECT_CALL(
      *memory_,
      storeBuffer(gsl::span<const uint8_t>(blake2b_256_result)))
      .WillOnce(Return(out_ptr));

  ASSERT_EQ(crypto_ext_->ext_hashing_blake2_256_version_1(
                PtrSize{data, size}.combine()),
            out_ptr);
}

/**
 * @given initialized crypto extension @and data, which can be keccak-hashed
 * @when hashing that data
 * @then resulting hash is correct
 */
TEST_F(CryptoExtensionTest, KeccakValid) {
  WasmPointer data = 0;
  WasmSize size = input.size();
  WasmPointer out_ptr = 42;

  EXPECT_CALL(*memory_, loadN(data, size)).WillOnce(Return(input));
  EXPECT_CALL(*memory_,
              storeBuffer(gsl::span<const uint8_t>(keccak_result)))
      .WillOnce(Return(out_ptr));

  ASSERT_EQ(out_ptr,
            crypto_ext_->ext_hashing_keccak_256_version_1(
                PtrSize{data, size}.combine()));
}

/**
 * @given initialized crypto extension @and ed25519-signed message
 * @when verifying signature of this message
 * @then verification is successful
 */
TEST_F(CryptoExtensionTest, Ed25519VerifySuccess) {
  auto keypair = ed25519_provider_->generateKeypair();
  EXPECT_OUTCOME_TRUE(signature, ed25519_provider_->sign(keypair, input));

  Buffer pubkey_buf(keypair.public_key);
  Buffer sig_buf(signature);

  WasmPointer input_data = 0;
  WasmSize input_size = input.size();
  WasmPointer sig_data_ptr = 42;
  WasmPointer pub_key_data_ptr = 123;

  EXPECT_CALL(*memory_, loadN(input_data, input_size)).WillOnce(Return(input));
  EXPECT_CALL(*memory_, loadN(pub_key_data_ptr, ed25519_constants::PUBKEY_SIZE))
      .WillOnce(Return(pubkey_buf));
  EXPECT_CALL(*memory_, loadN(sig_data_ptr, ed25519_constants::SIGNATURE_SIZE))
      .WillOnce(Return(sig_buf));

  ASSERT_EQ(
      crypto_ext_->ext_crypto_ed25519_verify_version_1(
          PtrSize{sig_data_ptr, ed25519_constants::SIGNATURE_SIZE}.combine(),
          PtrSize{input_data, input_size}.combine(),
          pub_key_data_ptr),
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
  auto keypair = ed25519_provider_->generateKeypair();
  Ed25519Signature invalid_signature;
  invalid_signature.fill(0x11);

  Buffer pubkey_buf(keypair.public_key);
  Buffer invalid_sig_buf(invalid_signature);

  WasmPointer input_data = 0;
  WasmSize input_size = input.size();
  WasmPointer sig_data_ptr = 42;
  WasmPointer pub_key_data_ptr = 123;

  EXPECT_CALL(*memory_, loadN(input_data, input_size)).WillOnce(Return(input));
  EXPECT_CALL(*memory_, loadN(pub_key_data_ptr, ed25519_constants::PUBKEY_SIZE))
      .WillOnce(Return(pubkey_buf));
  EXPECT_CALL(*memory_, loadN(sig_data_ptr, ed25519_constants::SIGNATURE_SIZE))
      .WillOnce(Return(invalid_sig_buf));

  ASSERT_EQ(
      crypto_ext_->ext_crypto_ed25519_verify_version_1(
          PtrSize{sig_data_ptr, ed25519_constants::SIGNATURE_SIZE}.combine(),
          PtrSize{input_data, input_size}.combine(),
          pub_key_data_ptr),
      CryptoExtension::kVerifyFail);
}

/**
 * @given initialized crypto extension @and sr25519-signed message
 * @when verifying signature of this message
 * @then verification is successful
 */
TEST_F(CryptoExtensionTest, Sr25519VerifySuccess) {
  auto pub_key = gsl::span<uint8_t>(sr25519_keypair.public_key);
  WasmPointer input_data = 0;
  WasmSize input_size = input.size();
  WasmPointer sig_data_ptr = 42;
  WasmPointer pub_key_data_ptr = 123;

  EXPECT_CALL(*memory_, loadN(input_data, input_size)).WillOnce(Return(input));
  EXPECT_CALL(*memory_, loadN(pub_key_data_ptr, sr25519_constants::PUBLIC_SIZE))
      .WillOnce(Return(Buffer(pub_key)));
  EXPECT_CALL(*memory_, loadN(sig_data_ptr, sr25519_constants::SIGNATURE_SIZE))
      .WillOnce(Return(Buffer(sr25519_signature)));

  ASSERT_EQ(
      crypto_ext_->ext_crypto_sr25519_verify_version_2(
          PtrSize{sig_data_ptr, sr25519_constants::SIGNATURE_SIZE}.combine(),
          PtrSize{input_data, input_size}.combine(),
          pub_key_data_ptr),
      CryptoExtension::kVerifySuccess);
}

/**
 * @given initialized crypto extension @and sr25519-signed message
 * @when verifying signature of this message
 * @then verification fails
 */
TEST_F(CryptoExtensionTest, Sr25519VerifyFailure) {
  auto pub_key = gsl::span<uint8_t>(sr25519_keypair.public_key);
  auto false_signature = Buffer(sr25519_signature);
  ++false_signature[0];
  ++false_signature[1];
  ++false_signature[2];
  ++false_signature[3];

  WasmPointer input_data = 0;
  WasmSize input_size = input.size();
  WasmPointer sig_data_ptr = 42;
  WasmPointer pub_key_data_ptr = 123;

  EXPECT_CALL(*memory_, loadN(input_data, input_size)).WillOnce(Return(input));
  EXPECT_CALL(*memory_, loadN(pub_key_data_ptr, sr25519_constants::PUBLIC_SIZE))
      .WillOnce(Return(Buffer(pub_key)));
  EXPECT_CALL(*memory_, loadN(sig_data_ptr, sr25519_constants::SIGNATURE_SIZE))
      .WillOnce(Return(false_signature));

  ASSERT_EQ(
      crypto_ext_->ext_crypto_sr25519_verify_version_2(
          PtrSize{sig_data_ptr, sr25519_constants::SIGNATURE_SIZE}.combine(),
          PtrSize{input_data, input_size}.combine(),
          pub_key_data_ptr),
      CryptoExtension::kVerifyFail);
}

/**
 * @given initialized crypto extention without started batch
 * @when trying to finish batch
 * @then exception is thrown
 */
TEST_F(CryptoExtensionTest, VerificationBatching_FinishWithoutStart) {
  ASSERT_THROW(crypto_ext_->ext_crypto_finish_batch_verify_version_1(),
               std::runtime_error);
}

/**
 * @given initialized crypto extention without started batch
 * @when trying to start batch twice
 * @then exception is thrown at second call
 */
TEST_F(CryptoExtensionTest, VerificationBatching_StartAgainWithoutFinish) {
  ASSERT_NO_THROW(crypto_ext_->ext_crypto_start_batch_verify_version_1());
  ASSERT_THROW(crypto_ext_->ext_crypto_start_batch_verify_version_1(),
               std::runtime_error);
}

/**
 * @given initialized crypto extention without started batch
 * @when start batch, check valid signature, and finish batch
 * @then verification returns positive, batch result is positive too
 */
TEST_F(CryptoExtensionTest, VerificationBatching_NormalOrderAndSuccess) {
  auto pub_key = gsl::span<uint8_t>(sr25519_keypair.public_key);
  auto valid_signature = Buffer(sr25519_signature);

  WasmPointer input_data = 0;
  WasmSize input_size = input.size();
  PtrSize input_span{input_data, input_size};
  WasmPointer sig_data_ptr = 42;
  WasmPointer pub_key_data_ptr = 123;

  EXPECT_CALL(*memory_, loadN(input_data, input_size)).WillOnce(Return(input));
  EXPECT_CALL(*memory_, loadN(pub_key_data_ptr, sr25519_constants::PUBLIC_SIZE))
      .WillOnce(Return(Buffer(pub_key)));
  EXPECT_CALL(*memory_, loadN(sig_data_ptr, sr25519_constants::SIGNATURE_SIZE))
      .WillOnce(Return(valid_signature));

  ASSERT_NO_THROW(crypto_ext_->ext_crypto_start_batch_verify_version_1());

  WasmSize result_in_place = crypto_ext_->ext_crypto_sr25519_verify_version_1(
      sig_data_ptr, input_span.combine(), pub_key_data_ptr);
  ASSERT_EQ(result_in_place, CryptoExtension::kVerifySuccess);

  WasmSize final_result;
  ASSERT_NO_THROW(final_result =
                      crypto_ext_->ext_crypto_finish_batch_verify_version_1());
  ASSERT_EQ(final_result, CryptoExtension::kVerifyBatchSuccess);
}

/**
 * @given initialized crypto extention without started batch
 * @when start batch, check valid signature, and finish batch
 * @then verification returns positive, but batch returns negative result
 */
TEST_F(CryptoExtensionTest, VerificationBatching_NormalOrderAndInvalid) {
  auto pub_key = gsl::span<uint8_t>(sr25519_keypair.public_key);
  auto valid_signature = Buffer(sr25519_signature);

  WasmPointer input_data = 0;
  WasmSize input_size = input.size();
  PtrSize input_span{input_data, input_size};
  WasmPointer sig_data_ptr = 42;
  WasmPointer pub_key_data_ptr = 123;

  EXPECT_CALL(*memory_, loadN(input_data, input_size)).WillOnce(Return(input));
  EXPECT_CALL(*memory_, loadN(pub_key_data_ptr, sr25519_constants::PUBLIC_SIZE))
      .WillOnce(Return(Buffer(pub_key)));
  EXPECT_CALL(*memory_, loadN(sig_data_ptr, sr25519_constants::SIGNATURE_SIZE))
      .WillOnce(Return(valid_signature));

  WasmSize result_in_place = crypto_ext_->ext_crypto_sr25519_verify_version_1(
      sig_data_ptr, input_span.combine(), pub_key_data_ptr);
  ASSERT_EQ(result_in_place, CryptoExtension::kVerifySuccess);

  ASSERT_ANY_THROW(crypto_ext_->ext_crypto_finish_batch_verify_version_1());
}

/**
 * @given initialized crypto extensions @and some bytes
 * @when XX-hashing those bytes to get 16-byte hash
 * @then resulting hash is correct
 */
TEST_F(CryptoExtensionTest, Twox128) {
  WasmPointer twox_input_data = 0;
  WasmSize twox_input_size = twox_input.size();
  WasmPointer out_ptr = 42;

  EXPECT_CALL(*memory_, loadN(twox_input_data, twox_input_size))
      .WillOnce(Return(twox_input));
  EXPECT_CALL(*memory_,
              storeBuffer(gsl::span<const uint8_t>(twox128_result)))
      .WillOnce(Return(out_ptr));

  ASSERT_EQ(out_ptr,
            crypto_ext_->ext_hashing_twox_128_version_1(
                PtrSize{twox_input_data, twox_input_size}.combine()));
}

/**
 * @given initialized crypto extensions @and some bytes
 * @when XX-hashing those bytes to get 32-byte hash
 * @then resulting hash is correct
 */
TEST_F(CryptoExtensionTest, Twox256) {
  WasmPointer twox_input_data = 0;
  WasmSize twox_input_size = twox_input.size();
  WasmPointer out_ptr = 42;

  EXPECT_CALL(*memory_, loadN(twox_input_data, twox_input_size))
      .WillOnce(Return(twox_input));
  EXPECT_CALL(*memory_,
              storeBuffer(gsl::span<const uint8_t>(twox256_result)))
      .WillOnce(Return(out_ptr));

  ASSERT_EQ(out_ptr,
            crypto_ext_->ext_hashing_twox_256_version_1(
                PtrSize{twox_input_data, twox_input_size}.combine()));
}

/**
 * @given initialized crypto extensions @and secp256k1 signature and message
 * @when call recovery public secp256k1 uncompressed key
 * @then resulting public key is correct
 */
TEST_F(CryptoExtensionTest, Secp256k1RecoverUncompressedSuccess) {
  WasmPointer sig = 1;
  WasmPointer msg = 10;
  WasmSpan res = PtrSize(20, 20).combine();
  auto &sig_input = secp_signature;
  auto &msg_input = secp_message_hash;

  EXPECT_CALL(*memory_, loadN(sig, sig_input.size()))
      .WillOnce(Return(Buffer(sig_input)));

  EXPECT_CALL(*memory_, loadN(msg, msg_input.size()))
      .WillOnce(Return(Buffer(msg_input)));

  EXPECT_CALL(*memory_,
              storeBuffer(gsl::span<const uint8_t>(
                  scale_encoded_secp_truncated_public_key)))
      .WillOnce(Return(res));

  auto ptrsize =
      crypto_ext_->ext_crypto_secp256k1_ecdsa_recover_version_1(sig, msg);
  ASSERT_EQ(ptrsize, res);
}

/**
 * @given initialized crypto extensions
 * @and a damaged secp256k1 signature and message
 * @when call recovery public secp256k1 uncompressed key
 * @then error code is returned
 */
TEST_F(CryptoExtensionTest, Secp256k1RecoverUncompressedFailure) {
  WasmPointer sig = 1;
  WasmPointer msg = 10;
  WasmSpan res = PtrSize(20, 20).combine();
  auto &sig_input = secp_signature;
  auto &msg_input = secp_message_hash;
  auto sig_buffer = Buffer(sig_input);
  // corrupt signature
  std::fill(sig_buffer.begin() + 2, sig_buffer.begin() + 10, 0xFF);
  EXPECT_CALL(*memory_, loadN(sig, sig_input.size()))
      .WillOnce(Return(sig_buffer));

  EXPECT_CALL(*memory_, loadN(msg, msg_input.size()))
      .WillOnce(Return(Buffer(msg_input)));

  EXPECT_CALL(
      *memory_,
      storeBuffer(gsl::span<const uint8_t>(secp_invalid_signature_error)))
      .WillOnce(Return(res));

  auto ptrsize =
      crypto_ext_->ext_crypto_secp256k1_ecdsa_recover_version_1(sig, msg);
  ASSERT_EQ(ptrsize, res);
}

/**
 * @given initialized crypto extensions @and secp256k1 signature and message
 * @when call recovery public secp256k1 compressed key
 * @then resulting public key is correct
 */
TEST_F(CryptoExtensionTest, Secp256k1RecoverCompressed) {
  WasmPointer sig = 1;
  WasmPointer msg = 10;
  WasmSpan res = PtrSize(20, 20).combine();
  auto &sig_input = secp_signature;
  auto &msg_input = secp_message_hash;

  EXPECT_CALL(*memory_, loadN(sig, sig_input.size()))
      .WillOnce(Return(Buffer(sig_input)));

  EXPECT_CALL(*memory_, loadN(msg, msg_input.size()))
      .WillOnce(Return(Buffer(msg_input)));

  EXPECT_CALL(*memory_,
              storeBuffer(gsl::span<const uint8_t>(
                  scale_encoded_secp_compressed_public_key)))
      .WillOnce(Return(res));

  auto ptrsize =
      crypto_ext_->ext_crypto_secp256k1_ecdsa_recover_compressed_version_1(sig,
                                                                           msg);
  ASSERT_EQ(ptrsize, res);
}

/**
 * @given initialized crypto extensions
 * @and a damaged secp256k1 signature and message
 * @when call recovery public secp256k1 compressed key
 * @then error code is returned
 */
TEST_F(CryptoExtensionTest, Secp256k1RecoverCompressedFailure) {
  WasmPointer sig = 1;
  WasmPointer msg = 10;
  WasmSpan res = PtrSize(20, 20).combine();

  auto &sig_input = secp_signature;
  auto &msg_input = secp_message_hash;
  Buffer sig_buffer(sig_input);
  // corrupt signature
  std::fill(sig_buffer.begin() + 2, sig_buffer.begin() + 10, 0xFF);

  EXPECT_CALL(*memory_, loadN(sig, sig_input.size()))
      .WillOnce(Return(sig_buffer));

  EXPECT_CALL(*memory_, loadN(msg, msg_input.size()))
      .WillOnce(Return(Buffer(msg_input)));

  EXPECT_CALL(
      *memory_,
      storeBuffer(gsl::span<const uint8_t>(secp_invalid_signature_error)))
      .WillOnce(Return(res));

  auto ptrsize =
      crypto_ext_->ext_crypto_secp256k1_ecdsa_recover_compressed_version_1(sig,
                                                                           msg);
  ASSERT_EQ(ptrsize, res);
}

/**
 * @given initialized crypto extension, key type
 * @when call ext_ed25519_public_keys_v1 of crypto extension
 * @then we get serialized set of existing ed25519 keys
 */
TEST_F(CryptoExtensionTest, Ed25519GetPublicKeysSuccess) {
  WasmSpan res = PtrSize(1, 2).combine();

  auto key_type = kagome::crypto::KEY_TYPE_BABE;
  kagome::runtime::WasmSize key_type_ptr = 42;

  EXPECT_CALL(*crypto_store_, getEd25519PublicKeys(key_type))
      .WillOnce(Return(ed_public_keys));

  EXPECT_CALL(*memory_, load32u(key_type_ptr)).WillOnce(Return(key_type));
  EXPECT_CALL(*memory_,
              storeBuffer(gsl::span<const uint8_t>(ed_public_keys_result)))
      .WillOnce(Return(res));

  ASSERT_EQ(crypto_ext_->ext_crypto_ed25519_public_keys_version_1(key_type_ptr),
            res);
}

/**
 * @given initialized crypto extension, key type
 * @when call ext_sr25519_public_keys_v1 of crypto extension
 * @then we get serialized set of existing sr25519 keys
 */
TEST_F(CryptoExtensionTest, Sr25519GetPublicKeysSuccess) {
  WasmSpan res = PtrSize(1, 2).combine();

  auto key_type_ptr = 42u;
  auto key_type = kagome::crypto::KEY_TYPE_BABE;

  EXPECT_CALL(*crypto_store_, getSr25519PublicKeys(key_type))
      .WillOnce(Return(sr_public_keys));

  EXPECT_CALL(*memory_, load32u(key_type_ptr)).WillOnce(Return(key_type));
  EXPECT_CALL(*memory_,
              storeBuffer(gsl::span<const uint8_t>(sr_public_keys_result)))
      .WillOnce(Return(res));

  ASSERT_EQ(crypto_ext_->ext_crypto_sr25519_public_keys_version_1(key_type_ptr),
            res);
}

/**
 * @given initialized crypto extension, key type, public key value and message
 * @when call ext_ed25519_sign_v1 of crypto extension
 * @then we get a valid signature
 */
TEST_F(CryptoExtensionTest, Ed25519SignSuccess) {
  kagome::runtime::WasmSize key_type = kagome::crypto::KEY_TYPE_BABE;
  kagome::runtime::WasmSize key_type_ptr = 42;
  kagome::runtime::WasmPointer key = 2;
  auto msg = PtrSize(3, 4).combine();
  auto res = PtrSize(5, 6).combine();

  // load public key
  EXPECT_CALL(*memory_, loadN(2, Ed25519PublicKey::size()))
      .WillOnce(Return(ed_public_key_buffer));
  // load message
  EXPECT_CALL(*memory_, loadN(3, 4)).WillOnce(Return(input));

  EXPECT_CALL(*crypto_store_,
              findEd25519Keypair(key_type, ed25519_keypair.public_key))
      .WillOnce(Return(ed25519_keypair));

  EXPECT_CALL(*memory_, load32u(key_type_ptr)).WillOnce(Return(key_type));
  EXPECT_CALL(*memory_,
              storeBuffer(gsl::span<const uint8_t>(ed25519_signature_result)))
      .WillOnce(Return(res));
  ASSERT_EQ(
      crypto_ext_->ext_crypto_ed25519_sign_version_1(key_type_ptr, key, msg),
      res);
}

/**
 * @given initialized crypto extension,
 * key type, not existing public key and message
 * @when call ext_ed25519_sign_v1 of crypto extension
 * @then we get a valid serialized error
 */
TEST_F(CryptoExtensionTest, Ed25519SignFailure) {
  kagome::runtime::WasmSize key_type = kagome::crypto::KEY_TYPE_BABE;
  kagome::runtime::WasmSize key_type_ptr = 42;
  kagome::runtime::WasmPointer key = 2;
  auto msg = PtrSize(3, 4).combine();
  auto res = PtrSize(5, 6).combine();

  // load public key
  EXPECT_CALL(*memory_, loadN(2, Ed25519PublicKey::size()))
      .WillOnce(Return(ed_public_key_buffer));
  // load message
  EXPECT_CALL(*memory_, loadN(3, 4)).WillOnce(Return(input));
  EXPECT_CALL(*memory_, load32u(key_type_ptr)).WillOnce(Return(key_type));

  EXPECT_CALL(*crypto_store_,
              findEd25519Keypair(key_type, ed25519_keypair.public_key))
      .WillOnce(Return(
          outcome::failure(kagome::crypto::CryptoStoreError::KEY_NOT_FOUND)));

  EXPECT_CALL(*memory_,
              storeBuffer(gsl::span<const uint8_t>(
                  ed_sr_signature_failure_result_buffer)))
      .WillOnce(Return(res));
  ASSERT_EQ(
      crypto_ext_->ext_crypto_ed25519_sign_version_1(key_type_ptr, key, msg),
      res);
}

/**
 * @given initialized crypto extension, key type, public key value and message
 * @when call ext_sr25519_sign_v1 of crypto extension
 * @then we get a valid signature
 */
TEST_F(CryptoExtensionTest, Sr25519SignSuccess) {
  kagome::runtime::WasmSize key_type = kagome::crypto::KEY_TYPE_BABE;
  kagome::runtime::WasmSize key_type_ptr = 42;
  kagome::runtime::WasmPointer key = 2;
  auto msg = PtrSize(3, 4).combine();
  auto res = PtrSize(5, 6).combine();

  // load public key
  EXPECT_CALL(*memory_, loadN(2, Sr25519PublicKey::size()))
      .WillOnce(Return(sr_public_key_buffer));
  // load message
  EXPECT_CALL(*memory_, loadN(3, 4)).WillOnce(Return(input));
  EXPECT_CALL(*memory_, load32u(key_type_ptr)).WillOnce(Return(key_type));

  EXPECT_CALL(*crypto_store_,
              findSr25519Keypair(key_type, sr25519_keypair.public_key))
      .WillOnce(Return(sr25519_keypair));

  EXPECT_CALL(
      *memory_,
      storeBuffer(VerifySr25519Signature(sr25519_provider_,
                                         gsl::span<const uint8_t>(input),
                                         sr25519_keypair.public_key)))
      .WillOnce(Return(res));

  ASSERT_EQ(
      crypto_ext_->ext_crypto_sr25519_sign_version_1(key_type_ptr, key, msg),
      res);
}

/**
 * @given initialized crypto extension,
 * key type, not existing public key and message
 * @when call ext_sr25519_sign_v1 of crypto extension
 * @then we get a valid serialized error
 */
TEST_F(CryptoExtensionTest, Sr25519SignFailure) {
  kagome::runtime::WasmSize key_type = kagome::crypto::KEY_TYPE_BABE;
  kagome::runtime::WasmSize key_type_ptr = 42;
  kagome::runtime::WasmPointer key = 2;
  auto msg = PtrSize(3, 4).combine();
  auto res = PtrSize(5, 6).combine();

  // load public key
  EXPECT_CALL(*memory_, loadN(2, Sr25519PublicKey::size()))
      .WillOnce(Return(sr_public_key_buffer));
  // load message
  EXPECT_CALL(*memory_, loadN(3, 4)).WillOnce(Return(input));
  EXPECT_CALL(*memory_, load32u(key_type_ptr)).WillOnce(Return(key_type));

  EXPECT_CALL(*crypto_store_,
              findSr25519Keypair(key_type, sr25519_keypair.public_key))
      .WillOnce(Return(
          outcome::failure(kagome::crypto::CryptoStoreError::KEY_NOT_FOUND)));

  EXPECT_CALL(*memory_,
              storeBuffer(gsl::span<const uint8_t>(
                  ed_sr_signature_failure_result_buffer)))
      .WillOnce(Return(res));
  ASSERT_EQ(
      crypto_ext_->ext_crypto_sr25519_sign_version_1(key_type_ptr, key, msg),
      res);
}

/**
 * @given initialized crypto extension, key type and hexified seed
 * @when call generate ed25519 keypair method of crypto extension
 * @then a new ed25519 keypair is successfully generated and stored
 */
TEST_F(CryptoExtensionTest, Ed25519GenerateByHexSeedSuccess) {
  kagome::runtime::WasmSize key_type = kagome::crypto::KEY_TYPE_BABE;
  kagome::runtime::WasmSize key_type_ptr = 42;
  kagome::runtime::WasmPointer res = 2;
  auto seed_ptr = PtrSize(3, 4).combine();

  EXPECT_CALL(*crypto_store_,
              generateEd25519Keypair(key_type, std::string_view(mnemonic)))
      .WillOnce(Return(ed25519_keypair));
  EXPECT_CALL(*memory_, loadN(3, 4)).WillOnce(Return(mnemonic_buffer));
  EXPECT_CALL(*memory_,
              storeBuffer(gsl::span<const uint8_t>(ed_public_key_buffer)))
      .WillOnce(Return(res));
  EXPECT_CALL(*memory_, load32u(key_type_ptr)).WillOnce(Return(key_type));
  ASSERT_EQ(res,
            crypto_ext_->ext_crypto_ed25519_generate_version_1(key_type_ptr,
                                                               seed_ptr));
}

/**
 * @given initialized crypto extension, key type and mnemonic phrase seed
 * @when call generate ed25519 keypair method of crypto extension
 * @then a new ed25519 keypair is successfully generated and stored
 */
TEST_F(CryptoExtensionTest, Ed25519GenerateByMnemonicSuccess) {
  kagome::runtime::WasmSize key_type = kagome::crypto::KEY_TYPE_BABE;
  kagome::runtime::WasmSize key_type_ptr = 42;
  kagome::runtime::WasmPointer res = 2;
  auto seed_ptr = PtrSize(3, 4).combine();

  EXPECT_CALL(*memory_, loadN(3, 4)).WillOnce(Return(mnemonic_buffer));
  EXPECT_CALL(*memory_,
              storeBuffer(gsl::span<const uint8_t>(ed_public_key_buffer)))
      .WillOnce(Return(res));
  EXPECT_CALL(*memory_, load32u(key_type_ptr)).WillOnce(Return(key_type));
  EXPECT_CALL(*crypto_store_,
              generateEd25519Keypair(key_type, std::string_view(mnemonic)))
      .WillOnce(Return(ed25519_keypair));
  ASSERT_EQ(res,
            crypto_ext_->ext_crypto_ed25519_generate_version_1(key_type_ptr,
                                                               seed_ptr));
}

/**
 * @given initialized crypto extension, key type and hexified seed
 * @when call generate sr25519 keypair method of crypto extension
 * @then a new sr25519 keypair is successfully generated and stored
 */
TEST_F(CryptoExtensionTest, Sr25519GenerateByHexSeedSuccess) {
  kagome::runtime::WasmSize key_type = kagome::crypto::KEY_TYPE_BABE;
  kagome::runtime::WasmSize key_type_ptr = 42;
  kagome::runtime::WasmPointer res = 2;
  auto seed_ptr = PtrSize(3, 4).combine();

  EXPECT_CALL(*memory_, loadN(3, 4)).WillOnce(Return(mnemonic_buffer));
  EXPECT_CALL(*memory_,
              storeBuffer(gsl::span<const uint8_t>(sr_public_key_buffer)))
      .WillOnce(Return(res));
  EXPECT_CALL(*memory_, load32u(key_type_ptr)).WillOnce(Return(key_type));

  EXPECT_CALL(*crypto_store_,
              generateSr25519Keypair(key_type, std::string_view(mnemonic)))
      .WillOnce(Return(sr25519_keypair));
  ASSERT_EQ(res,
            crypto_ext_->ext_crypto_sr25519_generate_version_1(key_type_ptr,
                                                               seed_ptr));
}

/**
 * @given initialized crypto extension, key type and mnemonic phrase seed
 * @when call generate sr25519 keypair method of crypto extension
 * @then a new sr25519 keypair is successfully generated and stored
 */
TEST_F(CryptoExtensionTest, Sr25519GenerateByMnemonicSuccess) {
  kagome::runtime::WasmSize key_type = kagome::crypto::KEY_TYPE_BABE;
  kagome::runtime::WasmSize key_type_ptr = 42;
  kagome::runtime::WasmPointer res = 2;
  auto seed_ptr = PtrSize(3, 4).combine();

  EXPECT_CALL(*memory_, loadN(3, 4)).WillOnce(Return(mnemonic_buffer));
  EXPECT_CALL(*memory_,
              storeBuffer(gsl::span<const uint8_t>(sr_public_key_buffer)))
      .WillOnce(Return(res));
  EXPECT_CALL(*memory_, load32u(key_type_ptr)).WillOnce(Return(key_type));
  EXPECT_CALL(*crypto_store_,
              generateSr25519Keypair(key_type, std::string_view(mnemonic)))
      .WillOnce(Return(sr25519_keypair));
  ASSERT_EQ(res,
            crypto_ext_->ext_crypto_sr25519_generate_version_1(key_type_ptr,
                                                               seed_ptr));
}

/**
 * @given initialized crypto extension @and data, which can be keccak-hashed
 * @when hashing that data
 * @then resulting hash is correct
 */
TEST_F(CryptoExtensionTest, Keccac256Version1_Success) {
  WasmPointer data = 0;
  WasmSize size = input.size();
  WasmPointer out_ptr = 42;
  WasmSpan data_span = PtrSize(data, size).combine();

  EXPECT_CALL(*memory_, loadN(data, size)).WillOnce(Return(input));
  EXPECT_CALL(*memory_, storeBuffer(gsl::span<const uint8_t>(keccak_result)))
      .WillOnce(Return(out_ptr));

  ASSERT_EQ(crypto_ext_->ext_hashing_keccak_256_version_1(data_span), out_ptr);
}

/**
 * @given initialized crypto extension @and data, which can be sha2_256-hashed
 * @when hashing that data
 * @then resulting hash is correct
 */
TEST_F(CryptoExtensionTest, Sha2_256Version1_Success) {
  WasmPointer data = 0;
  WasmSize size = input.size();
  WasmPointer out_ptr = 42;
  WasmSpan data_span = PtrSize(data, size).combine();

  EXPECT_CALL(*memory_, loadN(data, size)).WillOnce(Return(input));
  EXPECT_CALL(*memory_, storeBuffer(gsl::span<const uint8_t>(sha2_256_result)))
      .WillOnce(Return(out_ptr));

  ASSERT_EQ(crypto_ext_->ext_hashing_sha2_256_version_1(data_span), out_ptr);
}

/**
 * @given initialized crypto extension @and data, which can be blake2_128-hashed
 * @when hashing that data
 * @then resulting hash is correct
 */
TEST_F(CryptoExtensionTest, Blake2_128Version1_Success) {
  WasmPointer data = 0;
  WasmSize size = input.size();
  WasmPointer out_ptr = 42;
  WasmSpan data_span = PtrSize(data, size).combine();

  EXPECT_CALL(*memory_, loadN(data, size)).WillOnce(Return(input));
  EXPECT_CALL(*memory_,
              storeBuffer(gsl::span<const uint8_t>(blake2b_128_result)))
      .WillOnce(Return(out_ptr));

  ASSERT_EQ(crypto_ext_->ext_hashing_blake2_128_version_1(data_span), out_ptr);
}

/**
 * @given initialized crypto extension @and data, which can be blake2_256-hashed
 * @when hashing that data
 * @then resulting hash is correct
 */
TEST_F(CryptoExtensionTest, Blake2_256Version1_Success) {
  WasmPointer data = 0;
  WasmSize size = input.size();
  WasmPointer out_ptr = 42;
  WasmSpan data_span = PtrSize(data, size).combine();

  EXPECT_CALL(*memory_, loadN(data, size)).WillOnce(Return(input));
  EXPECT_CALL(*memory_,
              storeBuffer(gsl::span<const uint8_t>(blake2b_256_result)))
      .WillOnce(Return(out_ptr));

  ASSERT_EQ(crypto_ext_->ext_hashing_blake2_256_version_1(data_span), out_ptr);
}

/**
 * @given initialized crypto extension @and data, which can be twox_256-hashed
 * @when hashing that data
 * @then resulting hash is correct
 */
TEST_F(CryptoExtensionTest, Twox_256Version1_Success) {
  WasmPointer data = 0;
  WasmSize size = twox_input.size();
  WasmPointer out_ptr = 42;
  WasmSpan data_span = PtrSize(data, size).combine();

  EXPECT_CALL(*memory_, loadN(data, size)).WillOnce(Return(twox_input));
  EXPECT_CALL(*memory_, storeBuffer(gsl::span<const uint8_t>(twox256_result)))
      .WillOnce(Return(out_ptr));

  ASSERT_EQ(crypto_ext_->ext_hashing_twox_256_version_1(data_span), out_ptr);
}

/**
 * @given initialized crypto extension @and data, which can be twox_128-hashed
 * @when hashing that data
 * @then resulting hash is correct
 */
TEST_F(CryptoExtensionTest, Twox_128Version1_Success) {
  WasmPointer data = 0;
  WasmSize size = twox_input.size();
  WasmPointer out_ptr = 42;
  WasmSpan data_span = PtrSize(data, size).combine();

  EXPECT_CALL(*memory_, loadN(data, size)).WillOnce(Return(twox_input));
  EXPECT_CALL(*memory_, storeBuffer(gsl::span<const uint8_t>(twox128_result)))
      .WillOnce(Return(out_ptr));

  ASSERT_EQ(crypto_ext_->ext_hashing_twox_128_version_1(data_span), out_ptr);
}

/**
 * @given initialized crypto extension @and data, which can be twox_128-hashed
 * @when hashing that data
 * @then resulting hash is correct
 */
TEST_F(CryptoExtensionTest, Twox_64Version1_Success) {
  WasmPointer data = 0;
  WasmSize size = twox_input.size();
  WasmPointer out_ptr = 42;
  WasmSpan data_span = PtrSize(data, size).combine();

  EXPECT_CALL(*memory_, loadN(data, size)).WillOnce(Return(twox_input));
  EXPECT_CALL(*memory_, storeBuffer(gsl::span<const uint8_t>(twox64_result)))
      .WillOnce(Return(out_ptr));

  ASSERT_EQ(crypto_ext_->ext_hashing_twox_64_version_1(data_span), out_ptr);
}
