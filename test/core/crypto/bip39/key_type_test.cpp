/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <string_view>

#include "crypto/key_type.hpp"

using namespace kagome::crypto;

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
  auto [key_type, repr, success] = GetParam();
  auto &&key_type_str = decodeKeyTypeId(key_type);

  std::cout << "value = " << key_type << std::endl;

  if (success) {
    ASSERT_EQ(key_type_str, repr);
  } else {
    ASSERT_NE(key_type_str, repr);
  }
}

INSTANTIATE_TEST_CASE_P(
    KeyTypeTestCases,
    KeyTypeTest,
    ::testing::Values(good(supported_key_types::kBabe, "babe"),
                      good(supported_key_types::kGran, "gran"),
                      good(supported_key_types::kAcco, "acco"),
                      good(supported_key_types::kImon, "imon"),
                      good(supported_key_types::kAudi, "audi"),
                      bad(supported_key_types::kBabe - 1, "babe"),
                      bad(supported_key_types::kBabe + 10, "babe")));
