/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <outcome/outcome.hpp>

#include "common/buffer.hpp"

#include "libp2p/crypto/public_key.hpp"
#include "libp2p/crypto/public_key.hpp"
#include "libp2p/crypto/common.hpp"

using kagome::common::Buffer;
using namespace libp2p::crypto; // NOLINT

TEST(CryptoKeys, simple) {
  auto private_key = PrivateKey(common::KeyType::kUnspecified, Buffer{1,2,3});
  ASSERT_EQ(private_key.getBytes().toVector(), (std::vector<uint8_t>{1,2,3}));
}
