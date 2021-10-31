/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "scale/scale.hpp"
#include "testutil/outcome.hpp"

using kagome::scale::ByteArray;
using kagome::scale::decode;
using kagome::scale::DecodeError;
using kagome::scale::encode;
using kagome::scale::EncodeError;
using kagome::scale::ScaleDecoderStream;
using kagome::scale::ScaleEncoderStream;

// TODO(yuraz): PRE-119 refactor to parameterized tests
/**
 * @given variety of optional values
 * @when encodeOptional function is applied
 * @then expected result obtained
 */
TEST(Scale, encodeOptional) {
  // most simple case
  {
    ScaleEncoderStream s;
    ASSERT_NO_THROW((s << std::optional<uint8_t>{std::nullopt}));
    ASSERT_EQ(s.data(), (ByteArray{0}));
  }

  // encode existing uint8_t
  {
    ScaleEncoderStream s;
    ASSERT_NO_THROW((s << std::optional<uint8_t>{1}));
    ASSERT_EQ(s.data(), (ByteArray{1, 1}));
  }

  // encode negative int8_t
  {
    ScaleEncoderStream s;
    ASSERT_NO_THROW((s << std::optional<int8_t>{-1}));
    ASSERT_EQ(s.data(), (ByteArray{1, 255}));
  }

  // encode non-existing uint16_t
  {
    ScaleEncoderStream s;
    ASSERT_NO_THROW((s << std::optional<uint16_t>{std::nullopt}));
    ASSERT_EQ(s.data(), (ByteArray{0}));
  }

  // encode existing uint16_t
  {
    ScaleEncoderStream s;
    ASSERT_NO_THROW((s << std::optional<uint16_t>{511}));
    ASSERT_EQ(s.data(), (ByteArray{1, 255, 1}));
  }

  // encode existing uint32_t
  {
    ScaleEncoderStream s;
    ASSERT_NO_THROW((s << std::optional<uint32_t>{67305985}));
    ASSERT_EQ(s.data(), (ByteArray{1, 1, 2, 3, 4}));
  }
}

/**
 * @given byte stream containing series of encoded optional values
 * @when decodeOptional function sequencially applied
 * @then expected values obtained
 */
TEST(ScaleTest, DecodeOptionalSuccess) {
  // clang-format off
    auto bytes = ByteArray{
            0,              // first value
            1, 1,           // second value
            1, 255,         // third value
            0,              // fourth value
            1, 255, 1,      // fifth value
            1, 1, 2, 3, 4}; // sixth value
  // clang-format on

  auto stream = ScaleDecoderStream{bytes};

  // decode nullopt uint8_t
  {
    std::optional<uint8_t> opt;
    ASSERT_NO_THROW((stream >> opt));
    ASSERT_FALSE(opt.has_value());
  }

  // decode optional uint8_t
  {
    std::optional<uint8_t> opt;
    ASSERT_NO_THROW((stream >> opt));
    ASSERT_TRUE(opt.has_value());
    ASSERT_EQ(*opt, 1);
  }

  // decode optional negative int8_t
  {
    std::optional<int8_t> opt;
    ASSERT_NO_THROW((stream >> opt));
    ASSERT_TRUE(opt.has_value());
    ASSERT_EQ(*opt, -1);
  }

  // decode nullopt uint16_t
  // it requires 1 zero byte just like any other nullopt
  {
    std::optional<uint16_t> opt;
    ASSERT_NO_THROW((stream >> opt));
    ASSERT_FALSE(opt.has_value());
  }

  // decode optional uint16_t
  {
    std::optional<uint16_t> opt;
    ASSERT_NO_THROW((stream >> opt));
    ASSERT_TRUE(opt.has_value());
    ASSERT_EQ(*opt, 511);
  }

  // decode optional uint32_t
  {
    std::optional<uint32_t> opt;
    ASSERT_NO_THROW((stream >> opt));
    ASSERT_TRUE(opt.has_value());
    ASSERT_EQ(*opt, 67305985);
  }
}

/**
 * optional bool tests
 */

/**
 * @given optional bool values: true, false, nullopt
 * @when encode optional is applied
 * @then expected result obtained
 */
TEST(ScaleTest, EncodeOptionalBoolSuccess) {
  std::vector<std::optional<bool>> values = {true, false, std::nullopt};
  ScaleEncoderStream s;
  for (auto &&v : values) {
    ASSERT_NO_THROW((s << v));
  }
  ASSERT_EQ(s.data(), (ByteArray{1, 2, 0}));
}

/**
 * @brief helper struct for testing decode optional bool
 */
struct FourOptBools {
  std::optional<bool> b1;
  std::optional<bool> b2;
  std::optional<bool> b3;
  std::optional<bool> b4;
};

template <class Stream, typename = std::enable_if_t<Stream::is_decoder_stream>>
Stream &operator>>(Stream &s, FourOptBools &v) {
  return s >> v.b1 >> v.b2 >> v.b3 >> v.b4;
}

/**
 * @given byte array containing series of encoded optional bool values
 * where last byte is incorrect for optional bool type
 * @when scale::decode function is applied
 * @then DecoderError::UNEXPECTED_VALUE error is obtained
 */
TEST(Scale, DecodeOptionalBoolFail) {
  auto bytes = ByteArray{0, 1, 2, 3};

  EXPECT_OUTCOME_FALSE_2(err, decode<FourOptBools>(bytes))
  ASSERT_EQ(err.value(), static_cast<int>(DecodeError::UNEXPECTED_VALUE));
}

/**
 * @given byte array containing series of encoded optional bool values
 * @when scale::decode function is applied
 * @then obtained values meet expectations
 */
TEST(Scale, DecodeOptionalBoolSuccess) {
  auto bytes = ByteArray{0, 1, 2, 1};
  using optbool = std::optional<bool>;

  EXPECT_OUTCOME_TRUE(res, decode<FourOptBools>(bytes))
  ASSERT_EQ(res.b1, std::nullopt);
  ASSERT_EQ(res.b2, optbool(true));
  ASSERT_EQ(res.b3, optbool(false));
  ASSERT_EQ(res.b4, optbool(true));
}
