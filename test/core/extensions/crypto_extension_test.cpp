/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "extensions/impl/crypto_extension.hpp"

#include <algorithm>

#include <ed25519/ed25519.h>
#include <gtest/gtest.h>
#include <gsl/span>
#include "core/runtime/mock_memory.hpp"
#include "crypto/random_generator/boost_generator.hpp"
#include "crypto/sr25519/sr25519_provider_impl.hpp"
#include "testutil/literals.hpp"

using namespace kagome::extensions;
using kagome::common::Buffer;
using kagome::crypto::BoostRandomGenerator;
using kagome::crypto::CSPRNG;
using kagome::crypto::SR25519Keypair;
using kagome::crypto::SR25519Provider;
using kagome::crypto::SR25519ProviderImpl;
using kagome::crypto::SR25519PublicKey;
using kagome::crypto::SR25519SecretKey;
using kagome::crypto::SR25519Signature;
using kagome::runtime::MockMemory;
using kagome::runtime::SizeType;
using kagome::runtime::WasmPointer;

using ::testing::Return;

namespace sr25519_constants = kagome::crypto::constants::sr25519;

class CryptoExtensionTest : public ::testing::Test {
 public:
  void SetUp() override {
    memory_ = std::make_shared<MockMemory>();

    random_generator_ = std::make_shared<BoostRandomGenerator>();
    sr25519_provider_ =
        std::make_shared<SR25519ProviderImpl>(random_generator_);
    crypto_ext_ = std::make_shared<CryptoExtension>(memory_, sr25519_provider_);

    sr25519_keypair = sr25519_provider_->generateKeypair();
    sr25519_signature = sr25519_provider_->sign(sr25519_keypair, input);
  }

 protected:
  std::shared_ptr<MockMemory> memory_;
  std::shared_ptr<CSPRNG> random_generator_;
  std::shared_ptr<SR25519Provider> sr25519_provider_;
  std::shared_ptr<CryptoExtension> crypto_ext_;

  Buffer input{"6920616d2064617461"_unhex};

  //  std::array<uint8_t, sr25519_constants::SIGNATURE_SIZE>
  //  sr25519_signature{};
  SR25519Signature sr25519_signature{};
  //  std::array<uint8_t, sr25519_constants::KEYPAIR_SIZE> sr25519_keypair{};
  SR25519Keypair sr25519_keypair{};

  Buffer blake2b_result{
      "ba67336efd6a3df3a70eeb757860763036785c182ff4cf587541a0068d09f5b2"_unhex};

  Buffer twox_input{"414243444546"_unhex};

  Buffer twox128_result{
      184, 65, 176, 250, 243, 129, 181, 3, 77, 82, 63, 150, 129, 221, 191, 251};

  Buffer twox256_result{184, 65,  176, 250, 243, 129, 181, 3,   77,  82, 63,
                        150, 129, 221, 191, 251, 33,  226, 149, 136, 6,  232,
                        81,  118, 200, 28,  69,  219, 120, 179, 208, 237};
};

/**
 * @given initialized crypto extension @and data, which can be blake2-hashed
 * @when hashing that data
 * @then resulting hash is correct
 */
TEST_F(CryptoExtensionTest, Blake2Valid) {
  WasmPointer data = 0;
  SizeType size = input.size();
  WasmPointer out_ptr = 42;

  EXPECT_CALL(*memory_, loadN(data, size)).WillOnce(Return(input));
  EXPECT_CALL(*memory_, storeBuffer(out_ptr, blake2b_result)).Times(1);

  crypto_ext_->ext_blake2_256(data, size, out_ptr);
}

/**
 * @given initialized crypto extension @and ed25519-signed message
 * @when verifying signature of this message
 * @then verification is successful
 */
TEST_F(CryptoExtensionTest, Ed25519VerifySuccess) {
  private_key_t private_key{};
  public_key_t public_key{};
  ASSERT_NE(ed25519_create_keypair(&private_key, &public_key), 0);

  signature_t signature{};
  ed25519_sign(
      &signature, input.data(), input.size(), &public_key, &private_key);

  Buffer pubkey_buf(public_key.data);
  Buffer sig_buf(signature.data);

  WasmPointer input_data = 0;
  SizeType input_size = input.size();
  WasmPointer sig_data_ptr = 42;
  WasmPointer pub_key_data_ptr = 123;

  EXPECT_CALL(*memory_, loadN(input_data, input_size)).WillOnce(Return(input));
  EXPECT_CALL(*memory_, loadN(pub_key_data_ptr, ed25519_pubkey_SIZE))
      .WillOnce(Return(pubkey_buf));
  EXPECT_CALL(*memory_, loadN(sig_data_ptr, ed25519_signature_SIZE))
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
  private_key_t private_key{};
  public_key_t public_key{};
  ASSERT_NE(ed25519_create_keypair(&private_key, &public_key), 0);

  signature_t invalid_signature{};
  std::fill(invalid_signature.data,
            invalid_signature.data + ed25519_signature_SIZE,
            0x11);

  Buffer pubkey_buf(public_key.data);
  Buffer invalid_sig_buf(invalid_signature.data);

  WasmPointer input_data = 0;
  SizeType input_size = input.size();
  WasmPointer sig_data_ptr = 42;
  WasmPointer pub_key_data_ptr = 123;

  EXPECT_CALL(*memory_, loadN(input_data, input_size)).WillOnce(Return(input));
  EXPECT_CALL(*memory_, loadN(pub_key_data_ptr, ed25519_pubkey_SIZE))
      .WillOnce(Return(pubkey_buf));
  EXPECT_CALL(*memory_, loadN(sig_data_ptr, ed25519_signature_SIZE))
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
  SizeType input_size = input.size();
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
  SizeType input_size = input.size();
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
  SizeType twox_input_size = twox_input.size();
  WasmPointer out_ptr = 42;

  EXPECT_CALL(*memory_, loadN(twox_input_data, twox_input_size))
      .WillOnce(Return(twox_input));
  EXPECT_CALL(*memory_, storeBuffer(out_ptr, twox128_result)).Times(1);

  crypto_ext_->ext_twox_128(twox_input_data, twox_input_size, out_ptr);
}

/**
 * @given initialized crypto extensions @and some bytes
 * @when XX-hashing those bytes to get 32-byte hash
 * @then resulting hash is correct
 */
TEST_F(CryptoExtensionTest, Twox256) {
  WasmPointer twox_input_data = 0;
  SizeType twox_input_size = twox_input.size();
  WasmPointer out_ptr = 42;

  EXPECT_CALL(*memory_, loadN(twox_input_data, twox_input_size))
      .WillOnce(Return(twox_input));
  EXPECT_CALL(*memory_, storeBuffer(out_ptr, twox256_result)).Times(1);

  crypto_ext_->ext_twox_256(twox_input_data, twox_input_size, out_ptr);
}
