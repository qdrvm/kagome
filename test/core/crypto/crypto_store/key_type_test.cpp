/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <string_view>

#include "crypto/crypto_store/key_type.hpp"

using kagome::crypto::decodeKeyTypeIdFromStr;
using kagome::crypto::encodeKeyTypeIdToStr;
using kagome::crypto::KeyType;
using kagome::crypto::KeyTypeId;

namespace {
  std::tuple<KeyTypeId, std::string_view, bool> good(KeyType id,
                                                     std::string_view v) {
    return {id, v, true};
  }

  std::tuple<KeyTypeId, std::string_view, bool> bad(KeyType id,
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

TEST_P(KeyTypeTest, CheckIfKnown) {
  auto [repr, key_type_str, should_succeed] = GetParam();
  auto is_supported = KeyType::is_supported(repr);

  if (should_succeed) {
    ASSERT_TRUE(is_supported);
  } else {
    ASSERT_FALSE(is_supported);
  }
}

INSTANTIATE_TEST_SUITE_P(KeyTypeTestCases,
                         KeyTypeTest,
                         ::testing::Values(good(KeyType::BABE, "babe"),
                                           good(KeyType::GRANDPA, "gran"),
                                           good(KeyType::ACCOUNT, "acco"),
                                           good(KeyType::IM_ONLINE, "imon"),
                                           good(KeyType::AUTHORITY_DISCOVERY,
                                                "audi"),
                                           good(KeyType::KEY_TYPE_ASGN, "asgn"),
                                           good(KeyType::KEY_TYPE_PARA, "para"),
                                           bad((KeyType)0, "babe"),
                                           bad((KeyType)666, "babe")));
