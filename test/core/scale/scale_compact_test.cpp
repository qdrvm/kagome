/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "scale/byte_array_stream.hpp"
#include "scale/compact.hpp"
#include "scale/scale_encoder_stream.hpp"
#include "scale/scale_error.hpp"
#include "testutil/literals.hpp"

using kagome::common::Buffer;
using kagome::scale::BigInteger;
using kagome::scale::ByteArray;
using kagome::scale::ByteArrayStream;
using kagome::scale::ScaleEncoderStream;
using kagome::scale::compact::decodeInteger;

/**
 * value parameterized tests
 */
class CompactTest
    : public ::testing::TestWithParam<std::pair<BigInteger, ByteArray>> {
 public:
  static std::pair<BigInteger, ByteArray> pair(BigInteger v, ByteArray m) {
    return std::make_pair<BigInteger, ByteArray>(std::move(v), std::move(m));
  }

 protected:
  ScaleEncoderStream s;
};

/**
 * @given a value and corresponding buffer match of its encoding
 * @when value is encoded by means of ScaleEncoderStream
 * @then encoded value matches predefined buffer
 */
TEST_P(CompactTest, EncodeSuccess) {
  const auto &[value, match] = GetParam();
  ASSERT_NO_THROW((s << value));
  ASSERT_EQ(s.data(), match);
}

INSTANTIATE_TEST_CASE_P(
    CompactTestCases, CompactTest,
    ::testing::Values(
        // 0 is min compact integer value, negative values are not allowed
        CompactTest::pair(0, {0}),
        // 1 is encoded as 4
        CompactTest::pair(1, {4}),
        // max 1 byte value
        CompactTest::pair(63, {252}),
        // min 2 bytes value
        CompactTest::pair(64, {1, 1}),
        // some 2 bytes value
        CompactTest::pair(255, {253, 3}),
        // some 2 bytes value
        CompactTest::pair(511, {253, 7}),
        // max 2 bytes value
        CompactTest::pair(16383, {253, 255}),
        // min 4 bytes value
        CompactTest::pair(16384, {2, 0, 1, 0}),
        // some 4 bytes value
        CompactTest::pair(65535, {254, 255, 3, 0}),
        // max 4 bytes value
        CompactTest::pair(1073741823ul, {254, 255, 255, 255}),
        // some multibyte integer
        CompactTest::pair(
            BigInteger("1234567890123456789012345678901234567890"),
            {0b110111, 210, 10, 63, 206, 150, 95, 188, 172, 184, 243, 219, 192,
             117, 32, 201, 160, 3}),
        // min multibyte integer
        CompactTest::pair(1073741824, {0b00000011, 0, 0, 0, 64}),
        // max multibyte integer
        CompactTest::pair(
            BigInteger(
                "224945689727159819140526925384299092943484855915095831"
                "655037778630591879033574393515952034305194542857496045"
                "531676044756160413302774714984450425759043258192756735"),
            "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
            "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
            "FFFF"_unhex)));

/**
 * Negative tests
 */

/**
 * @given a negative value -1
 * (negative values are not supported by compact encoding)
 * @when trying to encode this value
 * @then obtain error
 */
TEST(ScaleCompactTest, EncodeNegativeIntegerFails) {
  BigInteger v(-1);
  ScaleEncoderStream out;
  ASSERT_ANY_THROW((out << v));
  ASSERT_EQ(out.data().size(), 0);  // nothing was written to buffer
}

/**
 * @given a BigInteger value exceeding the range supported by scale
 * @when encode it a directly as BigInteger
 * @then obtain kValueIsTooBig error
 */
TEST(ScaleCompactTest, EncodeOutOfRangeBigIntegerFails) {
  // try to encode out of range big integer value MAX_BIGINT + 1 == 2^536
  // too big value, even for big integer case
  // we are going to have kValueIsTooBig error
  BigInteger v(
      "224945689727159819140526925384299092943484855915095831"
      "655037778630591879033574393515952034305194542857496045"
      "531676044756160413302774714984450425759043258192756736");  // 2^536

  ScaleEncoderStream out;
  ASSERT_ANY_THROW((out << v));     // value is too big, it is not encoded
  ASSERT_EQ(out.data().size(), 0);  // nothing was written to buffer
}

/**
 * Decode Tests
 */

/**
 * @given byte array of correctly encoded number 0
 * @when apply decodeInteger
 * @then result matches expectations
 */
TEST(Scale, compactDecodeZero) {
  // decode 0
  auto bytes = ByteArray{
      0b00000000,  // 0
  };
  auto stream = ByteArrayStream{bytes};
  auto &&result = decodeInteger(stream);
  ASSERT_TRUE(result);
  ASSERT_EQ(result.value(), 0);
}

/**
 * @given byte array of correctly encoded number 1
 * @when apply decodeInteger
 * @then result matches expectation
 */
TEST(Scale, compactDecodeOne) {
  // decode 1
  auto bytes = ByteArray{
      0b00000100,  // 4
  };
  auto stream = ByteArrayStream{bytes};
  auto &&result = decodeInteger(stream);
  ASSERT_TRUE(result);
  ASSERT_EQ(result.value(), 1);
}

/**
 * @given byte array of correctly encoded number 63, which is max value for 1-st
 * case of encoding
 * @when apply decodeInteger
 * @then result matches expectation
 */
TEST(Scale, compactDecodeMaxUi8) {
  // decode MAX_UI8 == 63
  auto bytes = ByteArray{
      0b11111100,  // 252
  };
  auto stream = ByteArrayStream{bytes};
  auto &&result = decodeInteger(stream);
  ASSERT_TRUE(result);
  ASSERT_EQ(result.value(), 63);
}

/**
 * @given byte array of correctly encoded number 64, which is min value for 2-nd
 * case of encoding
 * @when apply decodeInteger
 * @then result matches expectation
 */
TEST(Scale, compactDecodeMinUi16) {
  // decode MIN_UI16 := MAX_UI8+1 == 64
  auto bytes = ByteArray{
      0b00000001,  // 1
      0b00000001   // 1
  };
  auto stream = ByteArrayStream{bytes};
  auto &&result = decodeInteger(stream);
  ASSERT_TRUE(result);
  ASSERT_EQ(result.value(), 64);
}

/**
 * @given byte array of correctly encoded number 2^14 - 1, which is max value
 * for 2-nd case of encoding
 * @when apply decodeInteger
 * @then result matches expectation
 */
TEST(Scale, compactDecodeMaxUi16) {
  // decode MAX_UI16 == 2^14 - 1 == 16383
  auto bytes = ByteArray{
      0b11111101,  // 253
      0b11111111   // 255
  };
  auto stream = ByteArrayStream{bytes};
  auto &&result = decodeInteger(stream);
  ASSERT_TRUE(result);
  ASSERT_EQ(result.value(), 16383);
}

/**
 * @given byte array of correctly encoded number 2^14, which is min value for
 * 3-rd case of encoding
 * @when apply decodeInteger
 * @then result matches expectation
 */
TEST(Scale, compactDecodeMinUi32) {
  // decode MIN_UI32 := MAX_UI16 + 1 = 2^14 = 16384
  auto bytes = ByteArray{
      0b00000010,  // 2
      0b00000000,  // 0
      0b00000001,  // 1
      0b00000000   // 0
  };
  auto stream = ByteArrayStream{bytes};
  auto &&result = decodeInteger(stream);
  ASSERT_TRUE(result);
  ASSERT_EQ(result.value(), 16384);
}

/**
 * @given byte array of correctly encoded number 2^30 - 1, which is max value
 * for 3-rd case of encoding
 * @when apply decodeInteger
 * @then result matches expectation
 */
TEST(Scale, compactDecodeMaxUi32) {
  // decode MAX_UI32 := 2^30 - 1 == 1073741823
  auto bytes = ByteArray{
      0b11111110,  // 254
      0b11111111,  // 255
      0b11111111,  // 255
      0b11111111   // 255
  };
  auto stream = ByteArrayStream{bytes};
  auto &&result = decodeInteger(stream);
  ASSERT_TRUE(result);
  ASSERT_EQ(result.value(), 1073741823);
}

/**
 * @given byte array of correctly encoded number 2^30 - 1, which is max value
 * for 4-th case of encoding
 * @when apply decodeInteger
 * @then result matches expectation
 */
TEST(Scale, compactDecodeMinBigInteger) {
  // decode MIN_BIG_INTEGER := 2^30
  auto bytes = ByteArray{3, 0, 0, 0, 64};
  auto stream = ByteArrayStream{bytes};
  auto &&result = decodeInteger(stream);
  ASSERT_TRUE(result);
  ASSERT_EQ(result.value(), 1073741824);
}

/**
 * @given incorrect byte array, which assumes 4-th case of encoding
 * @when apply decodeInteger
 * @then get kNotEnoughData error
 */
TEST(Scale, compactDecodeBigIntegerError) {
  auto bytes = ByteArray{255, 255, 255, 255};
  auto stream = ByteArrayStream{bytes};
  auto &&result = decodeInteger(stream);
  ASSERT_FALSE(result);
  ASSERT_EQ(result.error().value(),
            static_cast<int>(kagome::scale::DecodeError::NOT_ENOUGH_DATA));
}
