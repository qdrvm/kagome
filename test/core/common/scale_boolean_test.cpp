/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "common/result.hpp"
#include "common/scale/basic_stream.hpp"
#include "common/scale/boolean.hpp"

using namespace kagome;
using namespace kagome::common;
using namespace common::scale;

/**
 * @given bool values: true and false
 * @when encode them by fixedwidth::encodeBool function
 * @then obtain expected result each time
 */
TEST(Scale, fixedwidthEncodeBool) {
  ASSERT_EQ(boolean::encodeBool(true), (ByteArray{0x1}));
  ASSERT_EQ(boolean::encodeBool(false), (ByteArray{0x0}));
}

/**
 * @given byte array containing values {0, 1, 2}
 * @when fixedwidth::decodeBool function is applied sequentially
 * @then it returns false, true and kUnexpectedValue error correspondingly
 */
TEST(Scale, fixedwidthDecodeBool) {
  //  fixedwidth::DecodeBoolRes
  // decode false
  auto bytes = ByteArray{0x0, 0x1, 0x2};
  auto stream = BasicStream{bytes};
  boolean::decodeBool(stream).match(
      [](const boolean::DecodeBoolResult::ValueType &v) {
        ASSERT_EQ(v.value, false);
      },
      [](const boolean::DecodeBoolResult::ErrorType &) { FAIL(); });

  // decode true
  boolean::decodeBool(stream).match(
      [](const boolean::DecodeBoolResult::ValueType &v) {
        ASSERT_EQ(v.value, true);
      },
      [](const boolean::DecodeBoolResult::ErrorType &) { FAIL(); });

  // decode unexpected value, we are going to have kUnexpectedValue error
  boolean::decodeBool(stream).match(
      [](const boolean::DecodeBoolResult::ValueType &v) { FAIL(); },
      [](const boolean::DecodeBoolResult::ErrorType &e) {
        ASSERT_EQ(e.error, DecodeError::kUnexpectedValue);
      });
}

/**
 * @given tribool values false, true and indeterminate
 * @when fixedwidth::encodeTribool function is applied sequentially
 * @then it returns 0, 1 and 2 correspondingly
 */
TEST(Scale, fixedwidthEncodeTribool) {
  // encode none
  ASSERT_EQ(0x0, boolean::encodeTribool(false));

  // encode false
  ASSERT_EQ(0x1, boolean::encodeTribool(true));

  // encode true
  ASSERT_EQ(0x2, boolean::encodeTribool(indeterminate));
}

/**
 * @given byte array {0, 1, 2, 3}
 * @when decodeTribool function is applied sequentially
 * @then it returns false, true, indeterminate and kUnexpectedValue error as
 * expected
 */
TEST(Scale, fixedwidthDecodeTribool) {
  // decode none
  auto bytes = ByteArray{0x0, 0x1, 0x2, 0x3};
  auto stream = BasicStream{bytes};
  boolean::decodeTribool(stream).match(
      [](const boolean::DecodeTriboolResult::ValueType &v) {
        ASSERT_EQ(false, v.value);
      },
      [](const boolean::DecodeTriboolResult::ErrorType &e) { FAIL(); });

  // decode false
  boolean::decodeTribool(stream).match(
      [](const boolean::DecodeTriboolResult::ValueType &v) {
        ASSERT_EQ(true, v.value);
      },
      [](const boolean::DecodeTriboolResult::ErrorType &e) { FAIL(); });

  // decode true
  boolean::decodeTribool(stream).match(
      [](const boolean::DecodeTriboolResult::ValueType &v) {
        ASSERT_EQ(isIndeterminate(v.value), true);
      },
      [](const boolean::DecodeTriboolResult::ErrorType &e) { FAIL(); });

  // decode unexpected value
  boolean::decodeTribool(stream).match(
      [](const boolean::DecodeTriboolResult::ValueType &v) { FAIL(); },
      [](const boolean::DecodeTriboolResult::ErrorType &e) {
        ASSERT_EQ(DecodeError::kUnexpectedValue, e.error);
      });
}
