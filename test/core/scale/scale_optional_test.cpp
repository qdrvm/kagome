/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "scale/scale.hpp"
#include "testutil/outcome.hpp"

using kagome::scale::ByteArray;
using kagome::scale::decode;
using kagome::scale::encode;
using kagome::scale::ScaleDecoderStream;
using kagome::scale::ScaleEncoderStream;
using kagome::scale::DecodeError;
using kagome::scale::EncodeError;

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
  for (auto && v : values) {
    ASSERT_NO_THROW((s << v));
  }
  ASSERT_EQ(s.data(), (ByteArray{2, 1, 0}));
}

/**
 * @given stream containing series of encoded optionalBool values
 * @when decodeOptional<bool> function is applied
 * @then expected values obtained
 */
TEST(Scale, decodeOptionalBool) {
  auto bytes = ByteArray{0, 1, 2, 3};
  auto stream = ScaleDecoderStream{gsl::make_span(bytes)};
  using bool_type = std::optional<bool>;

  // decode none
  {
    EXPECT_OUTCOME_TRUE(res, decode<bool_type>(stream))
    ASSERT_FALSE(res.has_value());
  }

  // decode false
  {
    EXPECT_OUTCOME_TRUE(res, decode<bool_type>(stream))
    ASSERT_TRUE(res.has_value());
    ASSERT_FALSE(*res);
  }

  // decode true
  {
    EXPECT_OUTCOME_TRUE(res, decode<bool_type>(stream))
    ASSERT_TRUE(res.has_value());
    ASSERT_TRUE(*res);
  }

  // decode error unexpected value
  {
    EXPECT_OUTCOME_FALSE_2(err, decode<bool_type>(stream));
    ASSERT_EQ(err.value(), static_cast<int>(DecodeError::UNEXPECTED_VALUE));
  }

  // not enough data
  {
    EXPECT_OUTCOME_FALSE_2(err, decode<bool_type>(stream));
    ASSERT_EQ(err.value(), static_cast<int>(DecodeError::NOT_ENOUGH_DATA));
  }
}
