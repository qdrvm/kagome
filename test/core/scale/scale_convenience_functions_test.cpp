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

#include "scale/scale.hpp"

#include <gtest/gtest.h>
#include <testutil/outcome.hpp>

using kagome::scale::decode;
using kagome::scale::encode;

struct TestStruct {
  std::string a;
  int b;

  inline bool operator==(const TestStruct &rhs) const {
    return a == rhs.a && b == rhs.b;
  }
};

template <class Stream, typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const TestStruct &test_struct) {
  return s << test_struct.a << test_struct.b;
}

template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, TestStruct &test_struct) {
  return s >> test_struct.a >> test_struct.b;
}

/**
 * @given encoded TestStruct
 * @when it is decoded back
 * @then we get original TestStruct
 */
TEST(ScaleConvenienceFuncsTest, EncodeSingleValidArgTest) {
  TestStruct s1{"some_string", 42};

  EXPECT_OUTCOME_TRUE(encoded, encode(s1));
  EXPECT_OUTCOME_TRUE(decoded, decode<TestStruct>(encoded));

  ASSERT_EQ(decoded, s1);
}

/**
 * @given encoded string and integer using scale::encode with variadic template
 * @when we decode encoded vector back and put those fields to TestStruct
 * @then we get original string and int
 */
TEST(ScaleConvenienceFuncsTest, EncodeSeveralValidArgTest) {
  std::string expected_string = "some_string";
  int expected_int = 42;

  EXPECT_OUTCOME_TRUE(encoded, encode(expected_string, expected_int));
  EXPECT_OUTCOME_TRUE(decoded, decode<TestStruct>(encoded));

  ASSERT_EQ(decoded.a, expected_string);
  ASSERT_EQ(decoded.b, expected_int);
}
