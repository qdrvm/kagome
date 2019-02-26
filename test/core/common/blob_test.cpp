/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/blob.hpp"
#include <gtest/gtest.h>

using namespace kagome;

/**
 * @given hex string
 * @when create blob object from this string using fromHex method
 * @then blob object is created and contains expected byte representation of the
 * hex string
 */
TEST(BlobTest, CreateFromValidHex) {
  std::string hex32 = "00ff";
  std::array<uint8_t, 2> expected{0, 255};

  auto result = common::Blob<2>::fromHex(hex32);
  ASSERT_NO_THROW({
    auto blob = boost::get<expected::Value<common::Blob<2>>>(result).value;
    EXPECT_EQ(blob, expected);
  }) << "fromHex returned an error instead of value";
}

/**
 * @given non hex string
 * @when try to create a Blob using fromHex on that string
 * @then kNonHexInput error is returned
 */
TEST(BlobTest, CreateFromNonHex) {
  std::string not_hex = "nothex";

  auto result = common::Blob<2>::fromHex(not_hex);
  ASSERT_NO_THROW({
    auto error = boost::get<expected::Error<common::UnhexError>>(result).error;
    ASSERT_EQ(error, common::UnhexError::kNonHexInput);
  }) << "fromHex returned a value instead of error";
}

/**
 * @given string with odd length
 * @when try to create a Blob using fromHex on that string
 * @then kNotEnoughInput error is returned
 */
TEST(BlobTest, CreateFromOddLengthHex) {
  std::string odd_hex = "0a1";

  auto result = common::Blob<2>::fromHex(odd_hex);
  ASSERT_NO_THROW({
    auto error = boost::get<expected::Error<common::UnhexError>>(result).error;
    ASSERT_EQ(error, common::UnhexError::kNotEnoughInput);
  }) << "fromHex returned a value instead of error";
}

/**
 * @given string with odd length
 * @when try to create a Blob using fromHex on that string
 * @then kNotEnoughInput error is returned
 */
TEST(BlobTest, CreateFromWrongLendthHex) {
  std::string odd_hex = "00ff00";

  auto result = common::Blob<2>::fromHex(odd_hex);
  ASSERT_NO_THROW({
    auto error = boost::get<expected::Error<common::UnhexError>>(result).error;
    ASSERT_EQ(error, common::UnhexError::kWrongLengthInput);
  }) << "fromHex returned a value instead of error";
}
