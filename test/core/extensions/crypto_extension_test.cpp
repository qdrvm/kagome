/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <algorithm>
#include <array>

#include <ed25519/ed25519.h>
#include <gtest/gtest.h>
#include "extensions/impl/crypto_extension.hpp"

using namespace kagome::extensions;

class CryptoExtensionTest : public ::testing::Test {
 public:
  CryptoExtension crypto_ext{};

  static constexpr size_t input_size = 9;
  static constexpr std::array<uint8_t, input_size> input = {
      0x69, 0x20, 0x61, 0x6d, 0x20, 0x64, 0x61, 0x74, 0x61};

  static constexpr std::array<uint8_t, 32> blake2b_result = {
      0xba, 0x67, 0x33, 0x6e, 0xfd, 0x6a, 0x3d, 0xf3, 0xa7, 0x0e, 0xeb,
      0x75, 0x78, 0x60, 0x76, 0x30, 0x36, 0x78, 0x5c, 0x18, 0x2f, 0xf4,
      0xcf, 0x58, 0x75, 0x41, 0xa0, 0x06, 0x8d, 0x09, 0xf5, 0xb2};

  static constexpr size_t twox_input_size = 6;
  static constexpr std::array<uint8_t, twox_input_size> twox_input = {
      0x41, 0x42, 0x43, 0x44, 0x45, 0x46};
  static constexpr std::array<uint8_t, 16> twox128_result = {
      184, 65, 176, 250, 243, 129, 181, 3, 77, 82, 63, 150, 129, 221, 191, 251};
  static constexpr std::array<uint8_t, 32> twox256_result = {
      184, 65,  176, 250, 243, 129, 181, 3,   77,  82, 63,
      150, 129, 221, 191, 251, 33,  226, 149, 136, 6,  232,
      81,  118, 200, 28,  69,  219, 120, 179, 208, 237};
};

/**
 * @given initialized crypto extension @and data, which can be blake2-hashed
 * @when hashing that data
 * @then resulting hash is correct
 */
TEST_F(CryptoExtensionTest, Blake2Valid) {
  std::array<uint8_t, 32> hash{};
  crypto_ext.ext_blake2_256(input.data(), input_size, hash.data());

  ASSERT_EQ(hash, blake2b_result);
}

/**
 * @given initialized crypto extension @and ed25519-signed message
 * @when verifying signature of this message
 * @then verification is success
 */
TEST_F(CryptoExtensionTest, Ed25519VerifySuccess) {
  private_key_t private_key{};
  public_key_t public_key{};
  ASSERT_NE(ed25519_create_keypair(&private_key, &public_key), 0);

  signature_t signature{};
  ed25519_sign(&signature, input.data(), input_size, &public_key, &private_key);

  ASSERT_EQ(crypto_ext.ext_ed25519_verify(input.data(), input_size,
                                          signature.data, public_key.data),
            0);
}

/**
 * @given initialized crypto extension @and incorrect ed25519 signature for some
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
            invalid_signature.data + ed25519_signature_SIZE, 0x11);

  ASSERT_EQ(
      crypto_ext.ext_ed25519_verify(input.data(), input_size,
                                    invalid_signature.data, public_key.data),
      5);
}

/**
 * @given initialized crypto extensions @and some bytes
 * @when XX-hashing those bytes to get 16-byte hash
 * @then resulting hash is correct
 */
TEST_F(CryptoExtensionTest, Twox128) {
  std::array<uint8_t, 16> hash{};
  crypto_ext.ext_twox_128(twox_input.data(), twox_input_size, hash.data());

  ASSERT_EQ(hash, twox128_result);
}

/**
 * @given initialized crypto extensions @and some bytes
 * @when XX-hashing those bytes to get 32-byte hash
 * @then resulting hash is correct
 */
TEST_F(CryptoExtensionTest, Twox256) {
  std::array<uint8_t, 32> hash{};
  crypto_ext.ext_twox_256(twox_input.data(), twox_input_size, hash.data());

  ASSERT_EQ(hash, twox256_result);
}
