/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
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

namespace std {
  inline std::ostream &operator<<(std::ostream &s,
                                  const TestStruct &test_struct) {
    s << test_struct.a << test_struct.b;
    return s;
  }
}  // namespace std

template <class Stream>
Stream &operator<<(Stream &s, const TestStruct &test_struct) {
  return s << test_struct.a << test_struct.b;
}

template <class Stream>
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
