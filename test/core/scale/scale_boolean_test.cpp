/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "testutil/literals.hpp"
#include "scale/boolean.hpp"
#include "scale/byte_array_stream.hpp"
#include "scale/scale_encoder_stream.hpp"
#include "scale/scale_error.hpp"

using kagome::common::Buffer;
using kagome::scale::ByteArray;
using kagome::scale::ByteArrayStream;
using kagome::scale::DecodeError;
using kagome::scale::EncodeError;
using kagome::scale::indeterminate;
using kagome::scale::isIndeterminate;
using kagome::scale::ScaleEncoderStream;
using kagome::scale::boolean::decodeBool;
using kagome::scale::boolean::decodeTribool;
using kagome::scale::tribool;

/**
 * @given bool values: true and false
 * @when encode them by fixedwidth::encodeBool function
 * @then obtain expected result each time
 */
TEST(ScaleBoolTest, EncodeBoolSuccess) {
  {
    ScaleEncoderStream s;
    ASSERT_NO_THROW((s << true));
    ASSERT_EQ(s.getBuffer(), (Buffer{0x1}));
  }
  {
    ScaleEncoderStream s;
    ASSERT_NO_THROW((s << false));
    ASSERT_EQ(s.getBuffer(), (Buffer{0x0}));
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
  auto bytes = "000102"_unhex;
  auto stream = ByteArrayStream{bytes};
  auto &&res = decodeBool(stream);
  ASSERT_TRUE(res);           // success, not failure
  ASSERT_FALSE(res.value());  // actual value;

  auto &&res1 = decodeBool(stream);
  ASSERT_TRUE(res1);
  ASSERT_TRUE(res1.value());

  auto &&res2 = decodeBool(stream);
  ASSERT_FALSE(res2);
  ASSERT_EQ(res2.error().value(),
            static_cast<int>(DecodeError::UNEXPECTED_VALUE));
}

/**
 * @given tribool values false, true and indeterminate
 * @when fixedwidth::encodeTribool function is applied sequentially
 * @then it returns 0, 1 and 2 correspondingly
 */
TEST(ScaleEncoderStreamTest, EncodeTriboolSuccess) {
  {  // encode false
    ScaleEncoderStream s;
    ASSERT_NO_THROW((s << tribool(false)));
    ASSERT_EQ(s.getBuffer(), (Buffer{0x0}));
  }
  {  // encode true
    ScaleEncoderStream s;
    ASSERT_NO_THROW((s << tribool(true)));
    ASSERT_EQ(s.getBuffer(), (Buffer{0x1}));
  }
  {  // encode intederminate
    ScaleEncoderStream s;
    ASSERT_NO_THROW((s << tribool(indeterminate)));
    ASSERT_EQ(s.getBuffer(), (Buffer{0x2}));
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
  auto bytes = "00010203"_unhex;
  auto stream = ByteArrayStream{bytes};
  auto &&res = decodeTribool(stream);
  ASSERT_TRUE(res);
  ASSERT_FALSE(res.value());
  auto &&res1 = decodeTribool(stream);
  ASSERT_TRUE(res1);
  ASSERT_TRUE(res1.value());

  auto &&res2 = decodeTribool(stream);
  ASSERT_TRUE(res2);
  ASSERT_TRUE(isIndeterminate(res2.value()));

  auto &&res3 = decodeTribool(stream);
  ASSERT_FALSE(res3);
  ASSERT_EQ(res3.error().value(),
            static_cast<int>(DecodeError::UNEXPECTED_VALUE));
}
