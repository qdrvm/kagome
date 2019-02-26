/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/hexutil.hpp"
#include <gtest/gtest.h>

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

  std::vector<uint8_t> actual;
  ASSERT_NO_THROW(
      actual =
          boost::get<kagome::expected::Value<std::vector<uint8_t>>>(unhex(s))
              .value)
      << "unhex result does not contain expected std::vector<uint8_t>";

  std::vector<uint8_t> expected{0, 1, 2, 4, 8, 16, 32, 255};

  ASSERT_EQ(actual, expected);
}

/**
 * @given Hexencoded string of odd length
 * @when unhex
 * @then unhex result contains kNotEnoughInput error
 */
TEST(Common, Hexutil_UnhexOdd) {
  ASSERT_NO_THROW({
    UnhexErrors unhex_error =
        boost::get<kagome::expected::Error<UnhexErrors>>(unhex("0")).error;
    ASSERT_EQ(unhex_error, UnhexErrors::kNotEnoughInput);
  }) << "unhex did not return an error as expected";
}

/**
 * @given Hexencoded string with non-hex letter
 * @when unhex
 * @then unhex result contains kNonHexInput error
 */
TEST(Common, Hexutil_UnhexInvalid) {
  ASSERT_NO_THROW({
    UnhexErrors unhex_error =
        boost::get<kagome::expected::Error<UnhexErrors>>(unhex("keks")).error;
    ASSERT_EQ(unhex_error, UnhexErrors::kNonHexInput);
  }) << "unhex did not return an error as expected";
}
