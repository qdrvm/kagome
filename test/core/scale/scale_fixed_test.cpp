/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "scale/byte_array_stream.hpp"
#include "scale/fixedwidth.hpp"
#include "scale/scale_encoder_stream.hpp"

using namespace kagome;          // NOLINT
using namespace kagome::common;  // NOLINT
using namespace kagome::scale;   // NOLINT
using common::Buffer;
using kagome::scale::ScaleEncoderStream;

template <typename T>
class IntegerTest : public ::testing::TestWithParam<std::pair<T, Buffer>> {
 public:
  static std::pair<T, Buffer> make_pair(T value, const Buffer &match) {
    return std::make_pair(value, match);
  }

 protected:
  ScaleEncoderStream s;
};

/**
 * @brief class for testing int8_t encode and decode
 */
class Int8Test : public IntegerTest<int8_t> {};

/**
 * @given a number and match buffer
 * @when given number being encoded by ScaleEncoderStream
 * @then resulting buffer matches predefined one
 */
TEST_P(Int8Test, EncodeSuccess) {
  auto [value, match] = GetParam();
  ScaleEncoderStream s;
  ASSERT_NO_THROW((s << value));
  ASSERT_EQ(s.getBuffer(), match);
}

INSTANTIATE_TEST_CASE_P(
    Int8TestCases, Int8Test,
    ::testing::Values(Int8Test::make_pair(0, Buffer{0}),
                      Int8Test::make_pair(-1, Buffer{255}),
                      Int8Test::make_pair(-128, Buffer{128}),
                      Int8Test::make_pair(-127, Buffer{129}),
                      Int8Test::make_pair(123, Buffer{123}),
                      Int8Test::make_pair(-15, Buffer{241})));

/**
 * @brief class for testing uint8_t encode and decode
 */
class Uint8Test : public IntegerTest<uint8_t> {};

/**
 * @given a number and match buffer
 * @when given number being encoded by ScaleEncoderStream
 * @then resulting buffer matches predefined one
 */
TEST_P(Uint8Test, EncodeSuccess) {
  auto [value, match] = GetParam();
  ScaleEncoderStream s;
  ASSERT_NO_THROW((s << value));
  ASSERT_EQ(s.getBuffer(), match);
}

INSTANTIATE_TEST_CASE_P(
    Uint8TestCases, Uint8Test,
    ::testing::Values(Uint8Test::make_pair(0, Buffer{0}),
                      Uint8Test::make_pair(234, Buffer{234}),
                      Uint8Test::make_pair(255, Buffer{255})));

/**
 * @brief class for testing int16_t encode and decode
 */
class Int16Test : public IntegerTest<int16_t> {};

/**
 * @given a number and match buffer
 * @when given number being encoded by ScaleEncoderStream
 * @then resulting buffer matches predefined one
 */
TEST_P(Int16Test, EncodeSuccess) {
  auto [value, match] = GetParam();
  ScaleEncoderStream s;
  ASSERT_NO_THROW((s << value));
  ASSERT_EQ(s.getBuffer(), match);
}

INSTANTIATE_TEST_CASE_P(
    Int16TestCases, Int16Test,
    ::testing::Values(Int16Test::make_pair(-32767, Buffer{1, 128}),
                      Int16Test::make_pair(-32768, Buffer{0, 128}),
                      Int16Test::make_pair(-1, Buffer{255, 255}),
                      Int16Test::make_pair(32767, Buffer{255, 127}),
                      Int16Test::make_pair(12345, Buffer{57, 48}),
                      Int16Test::make_pair(-12345, Buffer{199, 207})));

/**
 * @brief class for testing uint16_t encode and decode
 */
class Uint16Test : public IntegerTest<uint16_t> {};

/**
 * @given a number and match buffer
 * @when given number being encoded by ScaleEncoderStream
 * @then resulting buffer matches predefined one
 */
TEST_P(Uint16Test, EncodeSuccess) {
  auto [value, match] = GetParam();
  ScaleEncoderStream s;
  ASSERT_NO_THROW((s << value));
  ASSERT_EQ(s.getBuffer(), match);
}

INSTANTIATE_TEST_CASE_P(
    Uint16TestCases, Uint16Test,
    ::testing::Values(Uint16Test::make_pair(32767, Buffer{255, 127}),
                      Uint16Test::make_pair(12345, Buffer{57, 48})));

/**
 * @brief class for testing int32_t encode and decode
 */
class Int32Test : public IntegerTest<int32_t> {};

/**
 * @given a number and match buffer
 * @when given number being encoded by ScaleEncoderStream
 * @then resulting buffer matches predefined one
 */
TEST_P(Int32Test, EncodeSuccess) {
  auto [value, match] = GetParam();
  ScaleEncoderStream s;
  ASSERT_NO_THROW((s << value));
  ASSERT_EQ(s.getBuffer(), match);
}

INSTANTIATE_TEST_CASE_P(
    Int32TestCases, Int32Test,
    ::testing::Values(Int32Test::make_pair(2147483647l,
                                           Buffer{255, 255, 255, 127}),
                      Int32Test::make_pair(-1, Buffer{255, 255, 255, 255})));

/**
 * @brief class for testing uint32_t encode and decode
 */
class Uint32Test : public IntegerTest<uint32_t> {};

/**
 * @given a number and match buffer
 * @when given number being encoded by ScaleEncoderStream
 * @then resulting buffer matches predefined one
 */
TEST_P(Uint32Test, EncodeSuccess) {
  auto [value, match] = GetParam();
  ScaleEncoderStream s;
  ASSERT_NO_THROW((s << value));
  ASSERT_EQ(s.getBuffer(), match);
}

INSTANTIATE_TEST_CASE_P(
    Uint32TestCases, Uint32Test,
    ::testing::Values(Uint32Test::make_pair(16909060ul, Buffer{4, 3, 2, 1}),
                      Uint32Test::make_pair(67305985, Buffer{1, 2, 3, 4})));

/**
 * @brief class for testing int64_t encode and decode
 */
class Int64Test : public IntegerTest<int64_t> {};

/**
 * @given a number and match buffer
 * @when given number being encoded by ScaleEncoderStream
 * @then resulting buffer matches predefined one
 */
TEST_P(Int64Test, EncodeSuccess) {
  auto [value, match] = GetParam();
  ScaleEncoderStream s;
  ASSERT_NO_THROW((s << value));
  ASSERT_EQ(s.getBuffer(), match);
}

INSTANTIATE_TEST_CASE_P(
    Int64TestCases, Int64Test,
    ::testing::Values(Int64Test::make_pair(578437695752307201ll,
                                           Buffer{1, 2, 3, 4, 5, 6, 7, 8}),
                      Int64Test::make_pair(
                          -1, Buffer{255, 255, 255, 255, 255, 255, 255, 255})));

/**
 * @brief class for testing uint64_t encode and decode
 */
class Uint64Test : public IntegerTest<uint64_t> {};

/**
 * @given a number and match buffer
 * @when given number being encoded by ScaleEncoderStream
 * @then resulting buffer matches predefined one
 */
TEST_P(Uint64Test, EncodeSuccess) {
  auto [value, match] = GetParam();
  ScaleEncoderStream s;
  ASSERT_NO_THROW((s << value));
  ASSERT_EQ(s.getBuffer(), match);
}

INSTANTIATE_TEST_CASE_P(
    Uint64TestCases, Uint64Test,
    ::testing::Values(Uint64Test::make_pair(578437695752307201ull,
                                            Buffer{1, 2, 3, 4, 5, 6, 7, 8})));

/**
 * @given byte array containing encoded int8_t values
 * @when fixedwidth::decodeInt8 is applied
 * @then correct sequence of decoded values is obtained
 */
TEST(Scale, fixedwidthDecodeInt8) {
  // decode int8_t
  auto bytes = Buffer{0, 255, 128, 129, 123, 241};
  auto stream = ByteArrayStream{bytes};

  {
    auto &&res = fixedwidth::decodeInt8(stream);
    ASSERT_TRUE(res);
    ASSERT_EQ(res.value(), 0);
  }
  {
    auto &&res = fixedwidth::decodeInt8(stream);
    ASSERT_TRUE(res);
    ASSERT_EQ(res.value(), -1);
  }
  {
    auto &&res = fixedwidth::decodeInt8(stream);
    ASSERT_TRUE(res);
    ASSERT_EQ(res.value(), -128);
  }
  {
    auto &&res = fixedwidth::decodeInt8(stream);
    ASSERT_TRUE(res);
    ASSERT_EQ(res.value(), -127);
  }
  {
    auto &&res = fixedwidth::decodeInt8(stream);
    ASSERT_TRUE(res);
    ASSERT_EQ(res.value(), 123);
  }
  {
    auto &&res = fixedwidth::decodeInt8(stream);
    ASSERT_TRUE(res);
    ASSERT_EQ(res.value(), -15);
  }
}

/**
 * @given byte array containing encoded uint8_t values
 * @when fixedwidth::decodeUint8 is applied
 * @then correct sequence of decoded values is obtained
 */
TEST(Scale, fixedwidthDecodeUint8) {
  // decode uint8_t
  auto bytes = Buffer{0, 234, 255};
  auto stream = ByteArrayStream{bytes};

  {
    auto &&res = fixedwidth::decodeUint8(stream);
    ASSERT_TRUE(res);
    ASSERT_EQ(res.value(), 0);
  }
  {
    auto &&res = fixedwidth::decodeUint8(stream);
    ASSERT_TRUE(res);
    ASSERT_EQ(res.value(), 234);
  }
  {
    auto &&res = fixedwidth::decodeUint8(stream);
    ASSERT_TRUE(res);
    ASSERT_EQ(res.value(), 255);
  }
}

/**
 * @given byte array containing encoded int16_t values
 * @when fixedwidth::decodeInt16 is applied
 * @then correct sequence of decoded values is obtained
 */
TEST(Scale, fixedwidthDecodeInt16) {
  // decode int16_t
  // clang-format off
    auto bytes = Buffer{1, 128,
                           0, 128,
                           255, 255,
                           255, 127,
                           57, 48,
                           199, 207};
  // clang-format on

  auto stream = ByteArrayStream{bytes};

  {
    auto &&res = fixedwidth::decodeInt16(stream);
    ASSERT_TRUE(res);
    ASSERT_EQ(res.value(), -32767);
  }
  {
    auto &&res = fixedwidth::decodeInt16(stream);
    ASSERT_TRUE(res);
    ASSERT_EQ(res.value(), -32768);
  }
  {
    auto &&res = fixedwidth::decodeInt16(stream);
    ASSERT_TRUE(res);
    ASSERT_EQ(res.value(), -1);
  }
  {
    auto &&res = fixedwidth::decodeInt16(stream);
    ASSERT_TRUE(res);
    ASSERT_EQ(res.value(), 32767);
  }
  {
    auto &&res = fixedwidth::decodeInt16(stream);
    ASSERT_TRUE(res);
    ASSERT_EQ(res.value(), 12345);
  }
  {
    auto &&res = fixedwidth::decodeInt16(stream);
    ASSERT_TRUE(res);
    ASSERT_EQ(res.value(), -12345);
  }

  //-0b11000000111001
}

/**
 * @given byte array containing encoded uint16_t values
 * @when fixedwidth::decodeUint16 is applied
 * @then correct sequence of decoded values is obtained
 */
TEST(Scale, fixedwidthDecodeUint16) {
  // decode uint16_t
  auto bytes = Buffer{2, 128};
  auto stream = ByteArrayStream{bytes};

  {
    auto &&res = fixedwidth::decodeUint16(stream);
    ASSERT_TRUE(res);
    ASSERT_EQ(res.value(), 32770);
  }
}

/**
 * @given byte array containing encoded int32_t values
 * @when fixedwidth::decodeInt32 is applied
 * @then correct sequence of decoded values is obtained
 */
TEST(Scale, fixedwidthDecodeInt32) {
  // decode int32_t
  // clang-format off
    auto bytes = Buffer{255, 255, 255, 127,
                           255, 255, 255, 255};
    //clang-format on

    auto stream = ByteArrayStream{bytes};

    {
        auto &&res = fixedwidth::decodeInt32(stream);
        ASSERT_TRUE(res);
        ASSERT_EQ(res.value(), 2147483647);
    }
    {
        auto &&res = fixedwidth::decodeInt32(stream);
        ASSERT_TRUE(res);
        ASSERT_EQ(res.value(), -1);
    }
}

/**
 * @given byte array containing encoded uint32_t values
 * @when fixedwidth::decodeUint32 is applied
 * @then correct sequence of decoded values is obtained
 */
TEST(Scale, fixedwidthDecodeUint32) {
    // decode uint32_t
    // clang-format off
    auto bytes = Buffer{4, 3, 2, 1,
                           1, 2, 3, 4};
    //clang-format on

    auto stream = ByteArrayStream{bytes};

    {
        auto &&res = fixedwidth::decodeUint32(stream);
        ASSERT_TRUE(res);
        ASSERT_EQ(res.value(), 16909060);
    }
    {
        auto &&res = fixedwidth::decodeUint32(stream);
        ASSERT_TRUE(res);
        ASSERT_EQ(res.value(), 67305985);
    }
}

/**
 * @given byte array containing encoded int64_t values
 * @when fixedwidth::decodeInt64 is applied
 * @then correct sequence of decoded values is obtained
 */
TEST(Scale, fixedwidthDecodeInt64) {
    // decode int64_t
    // clang-format off
    auto bytes = Buffer{1, 2, 3, 4, 5, 6, 7, 8,
                           255, 255, 255, 255, 255, 255, 255, 255};
    //clang-format on

    auto stream = ByteArrayStream{bytes};

    {
        auto &&res = fixedwidth::decodeInt64(stream);
        ASSERT_TRUE(res);
        ASSERT_EQ(res.value(), 578437695752307201);
    }
    {
        auto &&res = fixedwidth::decodeInt64(stream);
        ASSERT_TRUE(res);
        ASSERT_EQ(res.value(), -1);
    }
}

/**
 * @given byte array containing encoded uint64_t values
 * @when fixedwidth::decodeUint64 is applied
 * @then correct sequence of decoded values is obtained
 */
TEST(Scale, fixedwidthDecodeUint64) {
    // decode uint64_t
    // clang-format off
    auto bytes = Buffer{1, 2, 3, 4, 5, 6, 7, 8,
                           255, 255, 255, 255, 255, 255, 255, 255};
    //clang-format on

    auto stream = ByteArrayStream{bytes};

    {
        auto &&res = fixedwidth::decodeUint64(stream);
        ASSERT_TRUE(res);
        ASSERT_EQ(res.value(), 578437695752307201);
    }
    {
        auto &&res = fixedwidth::decodeUint64(stream);
        ASSERT_TRUE(res);
        ASSERT_EQ(res.value(), 18446744073709551615ull);
    }
}
