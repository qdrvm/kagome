/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "extensions/impl/crypto_extension.hpp"

#include <algorithm>

#include <gtest/gtest.h>
#include <gsl/span>
#include "core/runtime/mock_memory.hpp"
#include "crypto/ed25519/ed25519_provider_impl.hpp"
#include "crypto/hasher/hasher_impl.hpp"
#include "crypto/random_generator/boost_generator.hpp"
#include "crypto/secp256k1/secp256k1_provider_impl.hpp"
#include "crypto/sr25519/sr25519_provider_impl.hpp"
#include "runtime/wasm_result.hpp"
#include "scale/scale.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using namespace kagome::extensions;
using kagome::common::Buffer;
using kagome::crypto::BoostRandomGenerator;
using kagome::crypto::CSPRNG;
using kagome::crypto::ED25519Provider;
using kagome::crypto::ED25519ProviderImpl;
using kagome::crypto::ED25519Signature;
using kagome::crypto::Hasher;
using kagome::crypto::HasherImpl;
using kagome::crypto::Secp256k1Provider;
using kagome::crypto::Secp256k1ProviderImpl;
using kagome::crypto::SR25519Keypair;
using kagome::crypto::SR25519Provider;
using kagome::crypto::SR25519ProviderImpl;
using kagome::crypto::SR25519PublicKey;
using kagome::crypto::SR25519SecretKey;
using kagome::crypto::SR25519Signature;
using kagome::crypto::secp256k1::CompressedPublicKey;
using kagome::crypto::secp256k1::ExpandedPublicKey;
using kagome::crypto::secp256k1::MessageHash;
using kagome::crypto::secp256k1::RSVSignature;
using kagome::runtime::MockMemory;
using kagome::runtime::WasmPointer;
using kagome::runtime::WasmResult;
using kagome::runtime::WasmSize;
using kagome::runtime::WasmSpan;

using ::testing::Return;

namespace sr25519_constants = kagome::crypto::constants::sr25519;
namespace ed25519_constants = kagome::crypto::constants::ed25519;

class CryptoExtensionTest : public ::testing::Test {
 public:
  void SetUp() override {
    memory_ = std::make_shared<MockMemory>();

    random_generator_ = std::make_shared<BoostRandomGenerator>();
    sr25519_provider_ =
        std::make_shared<SR25519ProviderImpl>(random_generator_);
    ed25519_provider_ = std::make_shared<ED25519ProviderImpl>();
    secp256k1_provider_ = std::make_shared<Secp256k1ProviderImpl>();
    hasher_ = std::make_shared<HasherImpl>();
    crypto_ext_ = std::make_shared<CryptoExtension>(memory_,
                                                    sr25519_provider_,
                                                    ed25519_provider_,
                                                    secp256k1_provider_,
                                                    hasher_);

    sr25519_keypair = sr25519_provider_->generateKeypair();
    sr25519_signature = sr25519_provider_->sign(sr25519_keypair, input).value();

    std::copy_n(secp_message_vector.begin(),
                secp_message_vector.size(),
                secp_message_hash.begin());
    std::copy_n(secp_public_key_bytes.begin(),
                secp_public_key_bytes.size(),
                secp_uncompressed_public_key.begin());
    std::copy_n(secp_public_key_compressed_bytes.begin(),
                secp_public_key_compressed_bytes.size(),
                secp_compressed_pyblic_key.begin());
    std::copy_n(secp_signature_bytes.begin(),
                secp_signature_bytes.size(),
                secp_signature.begin());

    EXPECT_OUTCOME_TRUE(tmp1,
                        kagome::scale::encode(secp_uncompressed_public_key));
    scale_encoded_secp_uncompressed_public_key = Buffer(std::move(tmp1));
    EXPECT_OUTCOME_TRUE(tmp2,
                        kagome::scale::encode(secp_compressed_pyblic_key));
    scale_encoded_secp_compressed_public_key = Buffer(std::move(tmp2));
  }

 protected:
  std::shared_ptr<MockMemory> memory_;
  std::shared_ptr<CSPRNG> random_generator_;
  std::shared_ptr<SR25519Provider> sr25519_provider_;
  std::shared_ptr<ED25519Provider> ed25519_provider_;
  std::shared_ptr<Secp256k1Provider> secp256k1_provider_;
  std::shared_ptr<Hasher> hasher_;
  std::shared_ptr<CryptoExtension> crypto_ext_;

  Buffer input{"6920616d2064617461"_unhex};

  SR25519Signature sr25519_signature{};
  SR25519Keypair sr25519_keypair{};

  Buffer blake2b_128_result{"de944c5c12e55ee9a07cf5bf4b674995"_unhex};

  Buffer blake2b_256_result{
      "ba67336efd6a3df3a70eeb757860763036785c182ff4cf587541a0068d09f5b2"_unhex};

  Buffer keccak_result{
      "65aac3ad8b88cb79396da4c8b6a8cb6b5b74b0f6534a3e4e5e8ad68658feccf4"_unhex};

  Buffer twox_input{"414243444546"_unhex};

  Buffer twox128_result{
      184, 65, 176, 250, 243, 129, 181, 3, 77, 82, 63, 150, 129, 221, 191, 251};

  Buffer twox256_result{184, 65,  176, 250, 243, 129, 181, 3,   77,  82, 63,
                        150, 129, 221, 191, 251, 33,  226, 149, 136, 6,  232,
                        81,  118, 200, 28,  69,  219, 120, 179, 208, 237};

  std::vector<uint8_t> secp_public_key_bytes{
      "04e32df42865e97135acfb65f3bae71bdc86f4d49150ad6a440b6f15878109880a0a2b2667f7e725ceea70c673093bf67663e0312623c8e091b13cf2c0f11ef652"_unhex};
  std::vector<uint8_t> secp_public_key_compressed_bytes{
      "02e32df42865e97135acfb65f3bae71bdc86f4d49150ad6a440b6f15878109880a"_unhex};
  std::vector<uint8_t> secp_signature_bytes{
      "90f27b8b488db00b00606796d2987f6a5f59ae62ea05effe84fef5b8b0e549984a691139ad57a3f0b906637673aa2f63d1f55cb1a69199d4009eea23ceaddc9301"_unhex};
  std::vector<uint8_t> secp_message_vector{
      "ce0677bb30baa8cf067c88db9811f4333d131bf8bcf12fe7065d211dce971008"_unhex};
  MessageHash secp_message_hash{};

  RSVSignature secp_signature{};
  ExpandedPublicKey secp_uncompressed_public_key{};
  CompressedPublicKey secp_compressed_pyblic_key{};
  Buffer scale_encoded_secp_uncompressed_public_key;
  Buffer scale_encoded_secp_compressed_public_key;
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
      storeBuffer(out_ptr, gsl::span<const uint8_t>(blake2b_128_result)))
      .Times(1);

  crypto_ext_->ext_blake2_128(data, size, out_ptr);
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
      storeBuffer(out_ptr, gsl::span<const uint8_t>(blake2b_256_result)))
      .Times(1);

  crypto_ext_->ext_blake2_256(data, size, out_ptr);
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
              storeBuffer(out_ptr, gsl::span<const uint8_t>(keccak_result)))
      .Times(1);

  crypto_ext_->ext_keccak_256(data, size, out_ptr);
}

/**
 * @given initialized crypto extension @and ed25519-signed message
 * @when verifying signature of this message
 * @then verification is successful
 */
TEST_F(CryptoExtensionTest, Ed25519VerifySuccess) {
  EXPECT_OUTCOME_TRUE(keypair, ed25519_provider_->generateKeypair());
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

  ASSERT_EQ(crypto_ext_->ext_ed25519_verify(
                input_data, input_size, sig_data_ptr, pub_key_data_ptr),
            0);
}

/**
 * @given initialized crypto extension @and incorrect ed25519 signature for
 some
 * message
 * @when verifying signature of this message
 * @then verification fails
 */
TEST_F(CryptoExtensionTest, Ed25519VerifyFailure) {
  EXPECT_OUTCOME_TRUE(keypair, ed25519_provider_->generateKeypair());
  ED25519Signature invalid_signature;
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

  ASSERT_EQ(crypto_ext_->ext_ed25519_verify(
                input_data, input_size, sig_data_ptr, pub_key_data_ptr),
            5);
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

  ASSERT_EQ(crypto_ext_->ext_sr25519_verify(
                input_data, input_size, sig_data_ptr, pub_key_data_ptr),
            0);
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

  ASSERT_EQ(crypto_ext_->ext_sr25519_verify(
                input_data, input_size, sig_data_ptr, pub_key_data_ptr),
            5);
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
              storeBuffer(out_ptr, gsl::span<const uint8_t>(twox128_result)))
      .Times(1);

  crypto_ext_->ext_twox_128(twox_input_data, twox_input_size, out_ptr);
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
              storeBuffer(out_ptr, gsl::span<const uint8_t>(twox256_result)))
      .Times(1);

  crypto_ext_->ext_twox_256(twox_input_data, twox_input_size, out_ptr);
}

/**
 * @given initialized crypto extensions @and secp256k1 signature and message
 * @when call recovery public secp256k1 uncompressed key
 * @then resulting public key is correct
 */
TEST_F(CryptoExtensionTest, Secp256k1RecoverUncompressed) {
  WasmPointer sig = 1;
  WasmPointer msg = 10;
  WasmSpan res = WasmResult(20, 20).combine();
  auto &sig_input = secp_signature;
  auto &msg_input = secp_message_hash;

  EXPECT_CALL(*memory_, loadN(sig, sig_input.size()))
      .WillOnce(Return(Buffer(sig_input)));

  EXPECT_CALL(*memory_, loadN(msg, msg_input.size()))
      .WillOnce(Return(Buffer(msg_input)));

  EXPECT_CALL(*memory_,
              storeBuffer(gsl::span<const uint8_t>(
                  scale_encoded_secp_uncompressed_public_key)))
      .WillOnce(Return(res));

  auto ptrsize = crypto_ext_->ext_crypto_secp256k1_ecdsa_recover_v1(sig, msg);
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
  WasmSpan res = WasmResult(20, 20).combine();
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
      crypto_ext_->ext_crypto_secp256k1_ecdsa_recover_compressed_v1(sig, msg);
  ASSERT_EQ(ptrsize, res);
}
