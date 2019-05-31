/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "scale/byte_array_stream.hpp"
#include "scale/scale.hpp"
#include "scale/scale_decoder_stream.hpp"
#include "scale/scale_encoder_stream.hpp"
#include "scale/scale_error.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using kagome::scale::ByteArray;
using kagome::scale::DecodeError;
using kagome::scale::EncodeError;
using kagome::scale::ScaleDecoderStream;
using kagome::scale::ScaleEncoderStream;

/**
 * @given bool values: true and false
 * @when encode them by fixedwidth::encodeBool function
 * @then obtain expected result each time
 */
TEST(ScaleBoolTest, EncodeBoolSuccess) {
  {
    ScaleEncoderStream s;
    ASSERT_NO_THROW((s << true));
    ASSERT_EQ(s.data(), (ByteArray{0x1}));
  }
  {
    ScaleEncoderStream s;
    ASSERT_NO_THROW((s << false));
    ASSERT_EQ(s.data(), (ByteArray{0x0}));
  }
}

/**
 * @given byte array containing values {0, 1, 2}
 * @when fixedwidth::decodeBool function is applied sequentially
 * @then it returns false, true and kUnexpectedValue error correspondingly,
 * and in the end no more bytes left in stream
 */
TEST(Scale, fixedwidthDecodeBool) {
  auto bytes = ByteArray{0, 1, 2};
  auto stream = ScaleDecoderStream(bytes);
  EXPECT_OUTCOME_TRUE(res, kagome::scale::decode<bool>(stream))
  ASSERT_FALSE(res);

  EXPECT_OUTCOME_TRUE(res1, kagome::scale::decode<bool>(stream))
  ASSERT_TRUE(res1);

  EXPECT_OUTCOME_FALSE_2(err, kagome::scale::decode<bool>(stream))
  ASSERT_EQ(err.value(), static_cast<int>(DecodeError::UNEXPECTED_VALUE));
  ASSERT_FALSE(stream.hasMore(1));  // no more bytes left
}
