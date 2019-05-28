/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "scale/byte_array_stream.hpp"
#include "scale/optional_bool.hpp"
#include "scale/scale_encoder_stream.hpp"

using namespace kagome;          // NOLINT
using namespace kagome::common;  // NOLINT
using namespace kagome::scale;   // NOLINT
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
    ASSERT_EQ(s.getBuffer(), (Buffer{0}));
  }

  // encode existing uint8_t
  {
    ScaleEncoderStream s;
    ASSERT_NO_THROW((s << std::optional<uint8_t>{1}));
    ASSERT_EQ(s.getBuffer(), (Buffer{1, 1}));
  }

  // encode negative int8_t
  {
    ScaleEncoderStream s;
    ASSERT_NO_THROW((s << std::optional<int8_t>{-1}));
    ASSERT_EQ(s.getBuffer(), (Buffer{1, 255}));
  }

  // encode non-existing uint16_t
  {
    ScaleEncoderStream s;
    ASSERT_NO_THROW((s << std::optional<uint16_t>{std::nullopt}));
    ASSERT_EQ(s.getBuffer(), (Buffer{0}));
  }

  // encode existing uint16_t
  {
    ScaleEncoderStream s;
    ASSERT_NO_THROW((s << std::optional<uint16_t>{511}));
    ASSERT_EQ(s.getBuffer(), (Buffer{1, 255, 1}));
  }

  // encode existing uint32_t
  {
    ScaleEncoderStream s;
    ASSERT_NO_THROW((s << std::optional<uint32_t>{67305985}));
    ASSERT_EQ(s.getBuffer(), (Buffer{1, 1, 2, 3, 4}));
  }
}

/**
 * @given optional bool values: true, false, nullopt
 * @when encode optional is applied
 * @then expected result obtained
 */
TEST(ScaleTest, EncodeOptionalBoolSuccess) {
  ScaleEncoderStream s;
  ASSERT_NO_THROW((s << std::optional<bool>(true) << std::optional<bool>(false)
                     << std::nullopt));
  ASSERT_EQ(s.getBuffer(), (Buffer{2, 1, 0}));
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

  auto stream = ByteArrayStream{bytes};

  // decode nullopt uint8_t
  {
    auto &&res = optional::decodeOptional<uint8_t>(stream);
    ASSERT_TRUE(res);
    ASSERT_FALSE(res.value().has_value());
  }

  // decode optional uint8_t
  {
    auto &&res = optional::decodeOptional<uint8_t>(stream);
    ASSERT_TRUE(res);
    ASSERT_TRUE(res.value().has_value());
    ASSERT_EQ(*res.value(), 1);
  }

  // decode optional negative int8_t
  {
    auto &&res = optional::decodeOptional<int8_t>(stream);
    ASSERT_TRUE(res);
    ASSERT_TRUE(res.value().has_value());
    ASSERT_EQ(*res.value(), -1);
  }

  // decode nullopt uint16_t
  // it requires 1 zero byte just like any other nullopt
  {
    auto &&res = optional::decodeOptional<uint16_t>(stream);
    ASSERT_TRUE(res);
    ASSERT_FALSE(res.value().has_value());
  }

  // decode optional uint16_t
  {
    auto &&res = optional::decodeOptional<uint16_t>(stream);
    ASSERT_TRUE(res);
    ASSERT_TRUE(res.value().has_value());
    ASSERT_EQ(*res.value(), 511);
  }

  // decode optional uint32_t
  {
    auto &&res = optional::decodeOptional<uint32_t>(stream);
    ASSERT_TRUE(res);
    ASSERT_TRUE(res.value().has_value());
    ASSERT_EQ(*res.value(), 67305985);
  }
}

/**
 * @given stream containing series of encoded optionalBool values
 * @when decodeOptional<bool> function is applied
 * @then expected values obtained
 */
TEST(Scale, decodeOptionalBool) {
  auto bytes = ByteArray{0, 1, 2, 3};
  auto stream = ByteArrayStream{bytes};

  // decode none
  {
    auto &&res = optional::decodeOptional<bool>(stream);
    ASSERT_TRUE(res);
    ASSERT_FALSE(res.value().has_value());
  }

  // decode false
  {
    auto &&res = optional::decodeOptional<bool>(stream);
    ASSERT_TRUE(res);
    ASSERT_TRUE(res.value().has_value());
    ASSERT_FALSE(*res.value());
  }

  // decode true
  {
    auto &&res = optional::decodeOptional<bool>(stream);
    ASSERT_TRUE(res);
    ASSERT_TRUE(res.value().has_value());
    ASSERT_TRUE(*res.value());
  }

  // decode error unexpected value
  {
    auto &&res = optional::decodeOptional<bool>(stream);
    ASSERT_FALSE(res);
    ASSERT_EQ(res.error().value(),
              static_cast<int>(DecodeError::UNEXPECTED_VALUE));
  }

  // not enough data
  {
    auto &&res = optional::decodeOptional<bool>(stream);
    ASSERT_FALSE(res);
    ASSERT_EQ(res.error().value(),
              static_cast<int>(DecodeError::NOT_ENOUGH_DATA));
  }
}
