/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <gtest/gtest.h>

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
 * @brief helper structure for testing scale::decode
 */
struct ThreeBooleans {
  bool b1 = false;
  bool b2 = false;
  bool b3 = false;
};

template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, ThreeBooleans &v) {
  return s >> v.b1 >> v.b2 >> v.b3;
}


/**
 * @given byte array containing values {0, 1, 2}
 * @when scale::decode function is applied sequentially
 * @then it returns false, true and kUnexpectedValue error correspondingly,
 * and in the end no more bytes left in stream
 */
TEST(Scale, fixedwidthDecodeBoolFail) {
  auto bytes = ByteArray{0, 1, 2};
  EXPECT_OUTCOME_FALSE_2(err, kagome::scale::decode<ThreeBooleans>(bytes))
  ASSERT_EQ(err.value(), static_cast<int>(DecodeError::UNEXPECTED_VALUE));
}

/**
 * @given byte array containing values {0, 1, 2}
 * @when scale::decode function is applied sequentially
 * @then it returns false, true and kUnexpectedValue error correspondingly,
 * and in the end no more bytes left in stream
 */
TEST(Scale, fixedwidthDecodeBoolSuccess) {
  auto bytes = ByteArray{0, 1, 0};
  EXPECT_OUTCOME_TRUE(res, kagome::scale::decode<ThreeBooleans>(bytes))
  ASSERT_EQ(res.b1, false);
  ASSERT_EQ(res.b2, true);
  ASSERT_EQ(res.b3, false);
}
