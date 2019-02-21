/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/hexutil.hpp"
#include <gtest/gtest.h>

#include <boost/algorithm/hex.hpp> // for exceptions

using namespace kagome::common;
using namespace std::string_literals;

/**
 * @given Array of bytes
 * @when hex it
 * @then hex matches expected encoding
 */
TEST(Common, Hexutil_Hex) {
  std::vector<uint8_t> bin{0, 1, 2, 4, 8, 16, 32, 255};
  auto hexed = hex(bin.data(), bin.size());
  ASSERT_EQ(hexed, "00010204081020FF"s);
}

/**
 * @given Hexencoded string of even length
 * @when unhex
 * @then no exception, result matches expected value
 */
TEST(Common, Hexutil_UnhexEven) {
  auto s = "00010204081020ff"s;
  auto actual = unhex(s);
  std::vector<uint8_t> expected{0, 1, 2, 4, 8, 16, 32, 255};
  ASSERT_EQ(actual, expected);
}

/**
 * @given Hexencoded string of odd length
 * @when unhex
 * @then get exception boost::algorithm::not_enough_input
 */
TEST(Common, Hexutil_UnhexOdd) {
  ASSERT_THROW({ unhex("0"); }, boost::algorithm::not_enough_input);
}

/**
 * @given Hexencoded string with non-hex letter
 * @when unhex
 * @then get exception boost::algorithm::non_hex_input
 */
TEST(Common, Hexutil_UnhexInvalid) {
  ASSERT_THROW({ unhex("kek"); }, boost::algorithm::non_hex_input);
}
