/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <outcome/outcome.hpp>

#include "common/buffer.hpp"

#include "libp2p/crypto/common.hpp"
#include "libp2p/crypto/public_key.hpp"

using kagome::common::Buffer;
using namespace libp2p::crypto;

/**
 * @given sequence of bytes
 * @when private key of unspecified type containing provided bytes is created
 * @then created key contatins provided sequence of bytes and it's type is
 * unspecified
 */
TEST(CryptoKeys, privateKey) {
  auto key = PrivateKey(common::KeyType::UNSPECIFIED, Buffer{1, 2, 3});
  ASSERT_EQ(key.getBytes().toVector(), (std::vector<uint8_t>{1, 2, 3}));
  ASSERT_EQ(key.getType(), common::KeyType::UNSPECIFIED);
}

TEST(CryptoKeys, publicKey) {
  auto key = PublicKey(common::KeyType::UNSPECIFIED, Buffer{1, 2, 3});
  ASSERT_EQ(key.getBytes().toVector(), (std::vector<uint8_t>{1, 2, 3}));
  ASSERT_EQ(key.getType(), common::KeyType::UNSPECIFIED);
}
