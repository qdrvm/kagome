/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <outcome/outcome.hpp>

#include "common/buffer.hpp"
#include "libp2p/crypto/common.hpp"
#include "libp2p/crypto/aes/aes_provider.hpp"

using kagome::common::Buffer;
using namespace libp2p::crypto;

class AesFixture : public testing::Test {
 public:
  using Aes128Secret = libp2p::crypto::common::Aes128Secret;
  using Aes256Secret = libp2p::crypto::common::Aes256Secret;

  AesFixture() {
    iv = {0x3d, 0xaf, 0xba, 0x42, 0x9d, 0x9e, 0xb4, 0x30,
          0xb4, 0x22, 0xda, 0x80, 0x2c, 0x9f, 0xac, 0x41};

    key_128 = {0x06, 0xa9, 0x21, 0x40, 0x36, 0xb8, 0xa1, 0x5b,
               0x51, 0x2e, 0x03, 0xd5, 0x34, 0x12, 0x00, 0x06};

    key_256 = {0x78, 0xda, 0xe3, 0x4b, 0xc0, 0xeb, 0xa8, 0x13, 0xc0, 0x9c, 0xec,
               0x5c, 0x87, 0x1f, 0x3c, 0xcb, 0x39, 0xdc, 0xbb, 0xe0, 0x4a, 0x2f,
               0xe1, 0x83, 0x7e, 0x16, 0x9f, 0xee, 0x89, 0x6a, 0xa2, 0x08};

    cipher_text_128 = {0xd4, 0x31, 0x30, 0xf6, 0x52, 0xc4, 0xc8, 0x1b,
                       0xe6, 0x2f, 0xdf, 0x5e, 0x72, 0xe4, 0x8c, 0xbc};

    cipher_text_256 = {0x58, 0x6a, 0x49, 0xb4, 0xba, 0x03, 0x36, 0xff, 0xe1,
                       0x30, 0xc5, 0xf2, 0x7b, 0x80, 0xd3, 0xc9, 0x91, 0x0d,
                       0x7f, 0x42, 0x26, 0x87, 0xa6, 0x0b, 0x1b, 0x83, 0x3c,
                       0xff, 0x3d, 0x9e, 0xcb, 0xe0, 0x3e, 0x4d, 0xb5, 0x65,
                       0x3a, 0x67, 0x1f, 0xb1, 0xa7, 0xb2};

    plain_text_128.put("Single block msg");
    plain_text_256.put("The fly got to the jam that's all the poem");
  }

  Buffer iv;
  Buffer key_128;
  Buffer key_256;
  Buffer cipher_text_128;
  Buffer cipher_text_256;
  Buffer plain_text_128;
  Buffer plain_text_256;

  aes::AesProvider provider;
};

/**
 * @given key, iv, plain text and encrypted text
 * @when encrypt aes-128-ctr is applied
 * @then result matches encrypted text
 */
TEST_F(AesFixture, encode_aes_ctr_128) {
  Aes128Secret secret;

  std::copy(key_128.begin(), key_128.end(), secret.key);
  std::copy(iv.begin(), iv.end(), secret.iv);

  auto &&result = provider.encrypt_128_ctr(secret, plain_text_128);
  ASSERT_TRUE(result);
  ASSERT_EQ(result.value(), cipher_text_128);
}

/**
 * @given key, iv, plain text and encrypted text
 * @when encrypt aes-256-ctr is applied
 * @then result matches encrypted text
 */
TEST_F(AesFixture, encode_aes_ctr_256) {
  Aes256Secret secret;

  std::copy(key_256.begin(), key_256.end(), secret.key);
  std::copy(iv.begin(), iv.end(), secret.iv);

  auto &&result = provider.encrypt_256_ctr(secret, plain_text_256); // NOLINT
  ASSERT_TRUE(result);
  ASSERT_EQ(result.value(), cipher_text_256);
}

/**
 * @given key, iv, plain text and encrypted text
 * @when decrypt aes-128-ctr is applied
 * @then result matches plain text
 */
TEST_F(AesFixture, decode_aes_ctr_128) {
  Aes128Secret secret;

  std::copy(key_128.begin(), key_128.end(), secret.key);
  std::copy(iv.begin(), iv.end(), secret.iv);

  auto &&result = provider.decrypt_128_ctr(secret, cipher_text_128);
  ASSERT_TRUE(result);
  ASSERT_EQ(result.value(), plain_text_128);
}

/**
 * @given key, iv, plain text and encrypted text
 * @when decrypt aes-256-ctr is applied
 * @then result matches plain text
 */
TEST_F(AesFixture, decode_aes_ctr_256) {
  Aes256Secret secret;

  std::copy(key_256.begin(), key_256.end(), secret.key);
  std::copy(iv.begin(), iv.end(), secret.iv);

  auto &&result = provider.decrypt_256_ctr(secret, cipher_text_256);
  ASSERT_TRUE(result);
  ASSERT_EQ(result.value(), plain_text_256);
}
