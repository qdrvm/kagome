/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "common/result.hpp"
#include "scale/boolean.hpp"
#include "scale/byte_array_stream.hpp"
#include "scale/scale_error.hpp"

using namespace kagome;          // NOLINT
using namespace kagome::common;  // NOLINT
using namespace kagome::scale;   // NOLINT

/**
 * @given bool values: true and false
 * @when encode them by fixedwidth::encodeBool function
 * @then obtain expected result each time
 */
TEST(Scale, fixedwidthEncodeBool) {
  {
    Buffer out;
    boolean::encodeBool(true, out);
    ASSERT_EQ(out.toVector(), (ByteArray{0x1}));
  }

  {
    Buffer out;
    boolean::encodeBool(false, out);
    ASSERT_EQ(out.toVector(), (ByteArray{0x0}));
  }
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
  auto stream = ByteArrayStream{bytes};
  auto &&res = boolean::decodeBool(stream);
  ASSERT_TRUE(res);           // success, not failure
  ASSERT_FALSE(res.value());  // actual value;

  auto &&res1 = boolean::decodeBool(stream);
  ASSERT_TRUE(res1);
  ASSERT_TRUE(res1.value());

  auto &&res2 = boolean::decodeBool(stream);
  ASSERT_FALSE(res2);
  ASSERT_EQ(res2.error().value(),
            static_cast<int>(DecodeError::kUnexpectedValue));
}

/**
 * @given tribool values false, true and indeterminate
 * @when fixedwidth::encodeTribool function is applied sequentially
 * @then it returns 0, 1 and 2 correspondingly
 */
TEST(Scale, fixedwidthEncodeTribool) {
  {
    // encode false
    Buffer out;
    boolean::encodeTribool(false, out);
    ASSERT_EQ(out.toVector(), (ByteArray{0x0}));
  }
  {
    // encode true
    Buffer out;
    boolean::encodeTribool(true, out);
    ASSERT_EQ(out.toVector(), (ByteArray{0x1}));
  }
  {
    // encode intederminate
    Buffer out;
    boolean::encodeTribool(indeterminate, out);
    ASSERT_EQ(out.toVector(), (ByteArray{0x2}));
  }
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
  auto stream = ByteArrayStream{bytes};
  auto &&res = boolean::decodeTribool(stream);
  ASSERT_TRUE(res);
  ASSERT_FALSE(res.value());
  auto &&res1 = boolean::decodeTribool(stream);
  ASSERT_TRUE(res1);
  ASSERT_TRUE(res1.value());

  auto &&res2 = boolean::decodeTribool(stream);
  ASSERT_TRUE(res2);
  ASSERT_TRUE(isIndeterminate(res2.value()));

  auto &&res3 = boolean::decodeTribool(stream);
  ASSERT_FALSE(res3);
  ASSERT_EQ(res3.error().value(),
            static_cast<int>(DecodeError::kUnexpectedValue));
}
