/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/hexutil.hpp"
#include <gtest/gtest.h>

#include <boost/algorithm/hex.hpp>

using namespace kagome::common;
using namespace std::string_literals;

TEST(Common, Hexutil_Hex) {
  std::vector<uint8_t> bin{0, 1, 2, 4, 8, 16, 32, 255};
  auto hexed = hex(bin.data(), bin.size());
  ASSERT_EQ(hexed, "00010204081020FF"s);
}

TEST(Common, Hexutil_UnhexEven) {
  auto s = "00010204081020ff"s;
  auto actual = unhex(s);
  std::vector<uint8_t> expected{0, 1, 2, 4, 8, 16, 32, 255};
  ASSERT_EQ(actual, expected);
}

TEST(Common, Hexutil_UnhexOdd) {
  ASSERT_THROW({ unhex("0"); }, boost::algorithm::not_enough_input);
}

TEST(Common, Hexutil_UnhexInvalid) {
  ASSERT_THROW({ unhex("kek"); }, boost::algorithm::non_hex_input);
}
