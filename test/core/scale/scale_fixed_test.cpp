/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "scale/byte_array_stream.hpp"
#include "scale/fixedwidth.hpp"
#include "scale/scale_encoder_stream.hpp"

using kagome::scale::ByteArray;
using kagome::scale::ByteArrayStream;
using kagome::scale::ScaleEncoderStream;

// will be removed soon
using kagome::scale::fixedwidth::decodeInt16;
using kagome::scale::fixedwidth::decodeInt32;
using kagome::scale::fixedwidth::decodeInt64;
using kagome::scale::fixedwidth::decodeInt8;
using kagome::scale::fixedwidth::decodeUint16;
using kagome::scale::fixedwidth::decodeUint32;
using kagome::scale::fixedwidth::decodeUint64;
using kagome::scale::fixedwidth::decodeUint8;

template <typename T>
class IntegerTest : public ::testing::TestWithParam<std::pair<T, ByteArray>> {
 public:
  static std::pair<T, ByteArray> make_pair(T value, const ByteArray &match) {
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
  ASSERT_EQ(s.data(), match);
}

INSTANTIATE_TEST_CASE_P(Int8TestCases, Int8Test,
                        ::testing::Values(Int8Test::make_pair(0, {0}),
                                          Int8Test::make_pair(-1, {255}),
                                          Int8Test::make_pair(-128, {128}),
                                          Int8Test::make_pair(-127, {129}),
                                          Int8Test::make_pair(123, {123}),
                                          Int8Test::make_pair(-15, {241})));

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
  ASSERT_EQ(s.data(), match);
}

INSTANTIATE_TEST_CASE_P(Uint8TestCases, Uint8Test,
                        ::testing::Values(Uint8Test::make_pair(0, {0}),
                                          Uint8Test::make_pair(234, {234}),
                                          Uint8Test::make_pair(255, {255})));

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
  ASSERT_EQ(s.data(), match);
}

INSTANTIATE_TEST_CASE_P(
    Int16TestCases, Int16Test,
    ::testing::Values(Int16Test::make_pair(-32767, {1, 128}),
                      Int16Test::make_pair(-32768, {0, 128}),
                      Int16Test::make_pair(-1, {255, 255}),
                      Int16Test::make_pair(32767, {255, 127}),
                      Int16Test::make_pair(12345, {57, 48}),
                      Int16Test::make_pair(-12345, {199, 207})));

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
  ASSERT_EQ(s.data(), match);
}

INSTANTIATE_TEST_CASE_P(
    Uint16TestCases, Uint16Test,
    ::testing::Values(Uint16Test::make_pair(32767, {255, 127}),
                      Uint16Test::make_pair(12345, {57, 48})));

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
  ASSERT_EQ(s.data(), match);
}

INSTANTIATE_TEST_CASE_P(
    Int32TestCases, Int32Test,
    ::testing::Values(Int32Test::make_pair(2147483647l, {255, 255, 255, 127}),
                      Int32Test::make_pair(-1, {255, 255, 255, 255})));

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
  ASSERT_EQ(s.data(), match);
}

INSTANTIATE_TEST_CASE_P(
    Uint32TestCases, Uint32Test,
    ::testing::Values(Uint32Test::make_pair(16909060ul, {4, 3, 2, 1}),
                      Uint32Test::make_pair(67305985, {1, 2, 3, 4})));

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
  ASSERT_EQ(s.data(), match);
}

INSTANTIATE_TEST_CASE_P(
    Int64TestCases, Int64Test,
    ::testing::Values(
        Int64Test::make_pair(578437695752307201ll, {1, 2, 3, 4, 5, 6, 7, 8}),
        Int64Test::make_pair(-1, {255, 255, 255, 255, 255, 255, 255, 255})));

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
  ASSERT_EQ(s.data(), match);
}

INSTANTIATE_TEST_CASE_P(Uint64TestCases, Uint64Test,
                        ::testing::Values(Uint64Test::make_pair(
                            578437695752307201ull, {1, 2, 3, 4, 5, 6, 7, 8})));

/**
 * @given byte array containing encoded int8_t values
 * @when decodeInt8 is applied
 * @then correct sequence of decoded values is obtained
 */
TEST(Scale, fixedwidthDecodeInt8) {
  // decode int8_t
  auto bytes = ByteArray{0, 255, 128, 129, 123, 241};
  auto stream = ByteArrayStream{bytes};

  {
    auto &&res = decodeInt8(stream);
    ASSERT_TRUE(res);
    ASSERT_EQ(res.value(), 0);
  }
  {
    auto &&res = decodeInt8(stream);
    ASSERT_TRUE(res);
    ASSERT_EQ(res.value(), -1);
  }
  {
    auto &&res = decodeInt8(stream);
    ASSERT_TRUE(res);
    ASSERT_EQ(res.value(), -128);
  }
  {
    auto &&res = decodeInt8(stream);
    ASSERT_TRUE(res);
    ASSERT_EQ(res.value(), -127);
  }
  {
    auto &&res = decodeInt8(stream);
    ASSERT_TRUE(res);
    ASSERT_EQ(res.value(), 123);
  }
  {
    auto &&res = decodeInt8(stream);
    ASSERT_TRUE(res);
    ASSERT_EQ(res.value(), -15);
  }
}

/**
 * @given byte array containing encoded uint8_t values
 * @when decodeUint8 is applied
 * @then correct sequence of decoded values is obtained
 */
TEST(Scale, fixedwidthDecodeUint8) {
  // decode uint8_t
  auto bytes = ByteArray{0, 234, 255};
  auto stream = ByteArrayStream{bytes};

  {
    auto &&res = decodeUint8(stream);
    ASSERT_TRUE(res);
    ASSERT_EQ(res.value(), 0);
  }
  {
    auto &&res = decodeUint8(stream);
    ASSERT_TRUE(res);
    ASSERT_EQ(res.value(), 234);
  }
  {
    auto &&res = decodeUint8(stream);
    ASSERT_TRUE(res);
    ASSERT_EQ(res.value(), 255);
  }
}

/**
 * @given byte array containing encoded int16_t values
 * @when decodeInt16 is applied
 * @then correct sequence of decoded values is obtained
 */
TEST(Scale, fixedwidthDecodeInt16) {
  // decode int16_t
  // clang-format off
    ByteArray bytes = {1, 128,
                           0, 128,
                           255, 255,
                           255, 127,
                           57, 48,
                           199, 207};
  // clang-format on

  auto stream = ByteArrayStream{bytes};

  {
    auto &&res = decodeInt16(stream);
    ASSERT_TRUE(res);
    ASSERT_EQ(res.value(), -32767);
  }
  {
    auto &&res = decodeInt16(stream);
    ASSERT_TRUE(res);
    ASSERT_EQ(res.value(), -32768);
  }
  {
    auto &&res = decodeInt16(stream);
    ASSERT_TRUE(res);
    ASSERT_EQ(res.value(), -1);
  }
  {
    auto &&res = decodeInt16(stream);
    ASSERT_TRUE(res);
    ASSERT_EQ(res.value(), 32767);
  }
  {
    auto &&res = decodeInt16(stream);
    ASSERT_TRUE(res);
    ASSERT_EQ(res.value(), 12345);
  }
  {
    auto &&res = decodeInt16(stream);
    ASSERT_TRUE(res);
    ASSERT_EQ(res.value(), -12345);
  }

  //-0b11000000111001
}

/**
 * @given byte array containing encoded uint16_t values
 * @when decodeUint16 is applied
 * @then correct sequence of decoded values is obtained
 */
TEST(Scale, fixedwidthDecodeUint16) {
  // decode uint16_t
  auto bytes = ByteArray{2, 128};
  auto stream = ByteArrayStream{bytes};

  {
    auto &&res = decodeUint16(stream);
    ASSERT_TRUE(res);
    ASSERT_EQ(res.value(), 32770);
  }
}

/**
 * @given byte array containing encoded int32_t values
 * @when decodeInt32 is applied
 * @then correct sequence of decoded values is obtained
 */
TEST(Scale, fixedwidthDecodeInt32) {
  // decode int32_t
  // clang-format off
    auto bytes = ByteArray{255, 255, 255, 127,
                           255, 255, 255, 255};
    //clang-format on

    auto stream = ByteArrayStream{bytes};

    {
        auto &&res = decodeInt32(stream);
        ASSERT_TRUE(res);
        ASSERT_EQ(res.value(), 2147483647);
    }
    {
        auto &&res = decodeInt32(stream);
        ASSERT_TRUE(res);
        ASSERT_EQ(res.value(), -1);
    }
}

/**
 * @given byte array containing encoded uint32_t values
 * @when decodeUint32 is applied
 * @then correct sequence of decoded values is obtained
 */
TEST(Scale, fixedwidthDecodeUint32) {
    // decode uint32_t
    // clang-format off
    auto bytes = ByteArray{4, 3, 2, 1,
                           1, 2, 3, 4};
    //clang-format on

    auto stream = ByteArrayStream{bytes};

    {
        auto &&res = decodeUint32(stream);
        ASSERT_TRUE(res);
        ASSERT_EQ(res.value(), 16909060);
    }
    {
        auto &&res = decodeUint32(stream);
        ASSERT_TRUE(res);
        ASSERT_EQ(res.value(), 67305985);
    }
}

/**
 * @given byte array containing encoded int64_t values
 * @when decodeInt64 is applied
 * @then correct sequence of decoded values is obtained
 */
TEST(Scale, fixedwidthDecodeInt64) {
    // decode int64_t
    // clang-format off
    auto bytes = ByteArray{1, 2, 3, 4, 5, 6, 7, 8,
                           255, 255, 255, 255, 255, 255, 255, 255};
    //clang-format on

    auto stream = ByteArrayStream{bytes};

    {
        auto &&res = decodeInt64(stream);
        ASSERT_TRUE(res);
        ASSERT_EQ(res.value(), 578437695752307201);
    }
    {
        auto &&res = decodeInt64(stream);
        ASSERT_TRUE(res);
        ASSERT_EQ(res.value(), -1);
    }
}

/**
 * @given byte array containing encoded uint64_t values
 * @when decodeUint64 is applied
 * @then correct sequence of decoded values is obtained
 */
TEST(Scale, fixedwidthDecodeUint64) {
    // decode uint64_t
    // clang-format off
    auto bytes = ByteArray{1, 2, 3, 4, 5, 6, 7, 8,
                           255, 255, 255, 255, 255, 255, 255, 255};
    //clang-format on

    auto stream = ByteArrayStream{bytes};

    {
        auto &&res = decodeUint64(stream);
        ASSERT_TRUE(res);
        ASSERT_EQ(res.value(), 578437695752307201);
    }
    {
        auto &&res = decodeUint64(stream);
        ASSERT_TRUE(res);
        ASSERT_EQ(res.value(), 18446744073709551615ull);
    }
}
