/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <string_view>

#include "crypto/crypto_store/key_type.hpp"

using kagome::crypto::KEY_TYPE_ACCO;
using kagome::crypto::KEY_TYPE_AUDI;
using kagome::crypto::KEY_TYPE_BABE;
using kagome::crypto::KEY_TYPE_GRAN;
using kagome::crypto::KEY_TYPE_LP2P;
using kagome::crypto::KEY_TYPE_IMON;

using kagome::crypto::decodeKeyTypeId;
using kagome::crypto::KeyTypeId;

namespace {
  std::tuple<KeyTypeId, std::string_view, bool> good(KeyTypeId id,
                                                     std::string_view v) {
    return {id, v, true};
  }

  std::tuple<KeyTypeId, std::string_view, bool> bad(KeyTypeId id,
                                                    std::string_view v) {
    return {id, v, false};
  }

}  // namespace

struct KeyTypeTest : public ::testing::TestWithParam<
                         std::tuple<KeyTypeId, std::string_view, bool>> {};

TEST_P(KeyTypeTest, DecodeSuccess) {
  auto [key_type, repr, should_succeed] = GetParam();
  auto &&key_type_str = decodeKeyTypeId(key_type);

  if (should_succeed) {
    ASSERT_EQ(key_type_str, repr);
  } else {
    ASSERT_NE(key_type_str, repr);
  }
}

INSTANTIATE_TEST_CASE_P(KeyTypeTestCases,
                        KeyTypeTest,
                        ::testing::Values(good(KEY_TYPE_BABE, "babe"),
                                          good(KEY_TYPE_GRAN, "gran"),
                                          good(KEY_TYPE_ACCO, "acco"),
                                          good(KEY_TYPE_IMON, "imon"),
                                          good(KEY_TYPE_AUDI, "audi"),
                                          good(KEY_TYPE_LP2P, "lp2p"),
                                          bad(KEY_TYPE_BABE - 5, "babe"),
                                          bad(KEY_TYPE_BABE + 1000, "babe")));
