/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <string_view>

#include "crypto/crypto_store/key_type.hpp"

using kagome::crypto::KEY_TYPE_ACCO;
using kagome::crypto::KEY_TYPE_ASGN;
using kagome::crypto::KEY_TYPE_AUDI;
using kagome::crypto::KEY_TYPE_BABE;
using kagome::crypto::KEY_TYPE_GRAN;
using kagome::crypto::KEY_TYPE_IMON;
using kagome::crypto::KEY_TYPE_LP2P;
using kagome::crypto::KEY_TYPE_PARA;

using kagome::crypto::decodeKeyTypeIdFromStr;
using kagome::crypto::encodeKeyTypeIdToStr;
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
  auto &&key_type_str = encodeKeyTypeIdToStr(key_type);

  if (should_succeed) {
    ASSERT_EQ(key_type_str, repr);
  } else {
    ASSERT_NE(key_type_str, repr);
  }
}

TEST_P(KeyTypeTest, EncodeSuccess) {
  auto [repr, key_type_str, should_succeed] = GetParam();
  auto key_type = decodeKeyTypeIdFromStr(std::string(key_type_str));

  if (should_succeed) {
    ASSERT_EQ(key_type, repr);
  } else {
    ASSERT_NE(key_type, repr);
  }
}

INSTANTIATE_TEST_SUITE_P(KeyTypeTestCases,
                         KeyTypeTest,
                         ::testing::Values(good(KEY_TYPE_BABE, "babe"),
                                           good(KEY_TYPE_GRAN, "gran"),
                                           good(KEY_TYPE_ACCO, "acco"),
                                           good(KEY_TYPE_IMON, "imon"),
                                           good(KEY_TYPE_AUDI, "audi"),
                                           good(KEY_TYPE_LP2P, "lp2p"),
                                           good(KEY_TYPE_ASGN, "asgn"),
                                           good(KEY_TYPE_PARA, "para"),
                                           bad(KEY_TYPE_BABE - 5, "babe"),
                                           bad(KEY_TYPE_BABE + 1000, "babe")));
