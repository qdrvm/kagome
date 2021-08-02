/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <string>

#include <boost/optional.hpp>
#include "scale/scale_encoder_stream.hpp"

using kagome::scale::ScaleEncoderStream;

class ScaleCounter : public ::testing::Test {
 public:
  ScaleCounter() : s(true) {}

 protected:
  ScaleEncoderStream s;
};

struct TestStruct {
  uint8_t x;
  std::string y;
};

template <class Stream, typename = std::enable_if_t<Stream::is_encoder_stream>>
Stream &operator<<(Stream &s, const TestStruct &v) {
  return s << v.x << v.y;
}

// helper for same kind checks
#define SIZE(bytes) ASSERT_EQ(s.size(), bytes)

/**
 * @given a bool
 * @when it gets scale encoded
 * @then the resulting stream size equals to expected
 */
TEST_F(ScaleCounter, Bool) {
  s << true;
  SIZE(1);
}

/**
 * @given a string
 * @when it gets scale encoded
 * @then the resulting stream size equals to expected
 */
TEST_F(ScaleCounter, String) {
  std::string value = "test string";
  s << value;
  SIZE(value.size() + 1);
}

/**
 * @given an empty optional
 * @when it gets scale encoded
 * @then the resulting stream size equals to expected
 */
TEST_F(ScaleCounter, EmptyOptional) {
  boost::optional<uint32_t> var = boost::none;
  s << var;
  SIZE(1);
}

/**
 * @given an optional with an uint32 value inside
 * @when it gets scale encoded
 * @then the resulting stream size equals to expected
 */
TEST_F(ScaleCounter, NonEmptyOptional) {
  boost::optional<uint32_t> var = 10;
  s << var;
  SIZE(5);
}

/**
 * @given a custom defined struct
 * @when it gets scale encoded
 * @then the resulting stream size equals to expected
 */
TEST_F(ScaleCounter, CustomStruct) {
  TestStruct st{.x = 10, .y = "test string"};
  s << st;
  SIZE(1 + st.y.size() + 1);
}
