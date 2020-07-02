/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <string_view>

#include "crypto/crypto_store/key_type.hpp"

using kagome::crypto::decodeKeyTypeId;
using kagome::crypto::KeyTypeId;

using namespace kagome::crypto::supported_key_types;

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
                        ::testing::Values(good(kBabe, "babe"),
                                          good(kGran, "gran"),
                                          good(kAcco, "acco"),
                                          good(kImon, "imon"),
                                          good(kAudi, "audi"),
                                          good(kLp2p, "lp2p"),
                                          bad(kBabe - 5, "babe"),
                                          bad(kBabe + 1000, "babe")));
