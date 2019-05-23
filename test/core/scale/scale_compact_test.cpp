/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "scale/byte_array_stream.hpp"
#include "scale/compact.hpp"
#include "scale/scale_error.hpp"
#include "testutil/literals.hpp"

using namespace kagome;          // NOLINT
using namespace kagome::common;  // NOLINT
using namespace kagome::scale;   // NOLINT

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
  auto &&result = compact::decodeInteger(stream);
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
  auto &&result = compact::decodeInteger(stream);
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
  auto &&result = compact::decodeInteger(stream);
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
  auto &&result = compact::decodeInteger(stream);
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
  auto &&result = compact::decodeInteger(stream);
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
  auto &&result = compact::decodeInteger(stream);
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
  auto &&result = compact::decodeInteger(stream);
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
  auto &&result = compact::decodeInteger(stream);
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
  auto &&result = compact::decodeInteger(stream);
  ASSERT_FALSE(result);
  ASSERT_EQ(result.error().value(),
            static_cast<int>(DecodeError::NOT_ENOUGH_DATA));
}

/**
 * @given max value of first category integer = 2^6 - 1 = 63
 * @when encode it by compact::encodeInteger
 * @then obtain expected result: 1 byte representation
 */
TEST(Scale, compactEncodeFirstCategory) {
  // encode MAX_UI8 := 63
  Buffer out;
  auto &&res = compact::encodeInteger(63, out);
  ASSERT_TRUE(res);
  ASSERT_EQ(out.toVector(), std::vector<uint8_t>({252}));
}

/**
 * @given several encoding cases which needs 2 byte representation
 * @when encode it a by compact::encodeInteger
 * @then obtain expected result: 2 bytes representation
 */
TEST(Scale, compactEncodeSecondCategory) {
  {
    // encode MIN_UI16 := MAX_UI8 + 1
    Buffer out;
    auto &&res = compact::encodeInteger(64, out);
    ASSERT_TRUE(res);
    ASSERT_EQ(out.toVector(), (ByteArray{1, 1}));
  }

  {
    // encode some UI16
    Buffer out;
    auto &&res = compact::encodeInteger(255, out);
    ASSERT_TRUE(res);
    ASSERT_EQ(out.toVector(), (ByteArray{253, 3}));
  }

  {
    // encode some other UI16
    Buffer out;
    auto &&res = compact::encodeInteger(511, out);
    ASSERT_TRUE(res);
    ASSERT_EQ(out.toVector(), (ByteArray{253, 7}));
  }

  {
    // encode MAX_UI16 := 2^14 - 1 = 16383
    Buffer out;
    auto &&res = compact::encodeInteger(16383, out);
    ASSERT_TRUE(res);
    ASSERT_EQ(out.toVector(), (ByteArray{253, 255}));
  }
}

/**
 * @given min, max and intermediate values of third category = {2^14, 2^16 - 1,
 * 2^30 - 1}
 * @when encode it a by compact::encodeInteger
 * @then obtain expected result: 4 bytes representation
 */
TEST(Scale, compactEncodeThirdCategory) {
  // encode MIN_UI32 := MAX_UI16 + 1 == 16384
  {
    Buffer out;
    ASSERT_TRUE(compact::encodeInteger(16384, out));
    ASSERT_EQ(out.toVector(), (ByteArray{2, 0, 1, 0}));
  }

  // encode some uint16_t value which requires 4 bytes
  {
    Buffer out;
    ASSERT_TRUE(compact::encodeInteger(65535, out));
    ASSERT_EQ(out.toVector(), (ByteArray{254, 255, 3, 0}));
  }

  // 2^30 - 1 is max 4 byte value
  // encode MAX_UI32 := 2^30 == 1073741823
  {
    Buffer out;
    ASSERT_TRUE(compact::encodeInteger(1073741823ul, out));
    ASSERT_EQ(out.toVector(), (ByteArray{254, 255, 255, 255}));
  }
}

/**
 * @given max value of first category = 2^6 - 1 = 63
 * @when encode it a directly as BigInteger
 * @then obtain expected result: 1 byte representation
 */
TEST(Scale, compactEncodeFirstCategoryBigInteger) {
  auto v = BigInteger("63");
  Buffer out;
  ASSERT_TRUE(compact::encodeInteger(v, out));
  ASSERT_EQ(out.toVector(), (ByteArray{252}));
}

/**
 * @given max value of second category = 2^14 - 1
 * @when encode it a directly as BigInteger
 * @then obtain expected result: 2 bytes representation
 */
TEST(Scale, compactEncodeSecondCategoryBigInteger) {
  BigInteger v("16383");  // 2^14 - 1
  Buffer out;
  ASSERT_TRUE(compact::encodeInteger(v, out));
  ASSERT_EQ(out.toVector(), (ByteArray{253, 255}));
}

/**
 * @given max value of third category = 2^30 - 1
 * @when encode it a directly as BigInteger
 * @then obtain expected result: 4 bytes representation
 */
TEST(Scale, compactEncodeThirdCategoryBigInteger) {
  BigInteger v("1073741823");  // 2^30 - 1
  Buffer out;
  ASSERT_TRUE(compact::encodeInteger(v, out));
  ASSERT_EQ(out.toVector(), (ByteArray{254, 255, 255, 255}));
}

/**
 * @given some value of fourth category
 * @when encode it a directly as BigInteger
 * @then obtain expected result: multibyte representation
 */
TEST(Scale, compactEncodeFourthCategoryBigInteger) {
  BigInteger v(
      "1234567890123456789012345678901234567890");  // (1234567890) x 4 times
  Buffer out;
  ASSERT_TRUE(compact::encodeInteger(v, out));
  ASSERT_EQ(out.toVector(),
            (ByteArray{0b110111, 210, 10, 63, 206, 150, 95, 188, 172, 184, 243,
                       219, 192, 117, 32, 201, 160, 3}));
}

/**
 * @given min value which must be encoded as 4-th case
 * @when encode it a directly as BigInteger
 * @then obtain expected result: multibyte representation
 */
TEST(Scale, compactEncodeMinBigInteger) {
  BigInteger v(1073741824);
  Buffer out;
  ASSERT_TRUE(compact::encodeInteger(v, out));
  ASSERT_EQ(out.toVector(), (ByteArray{0b00000011, 0, 0, 0, 64}));
}

/**
 * @given max value supported by scale
 * @when encode it a directly as BigInteger
 * @then obtain expected result: multibyte representation
 */
TEST(Scale, compactEncodeMaxBigInteger) {
  // encode max big integer value := 2^536 - 1
  BigInteger v(
      "224945689727159819140526925384299092943484855915095831"
      "655037778630591879033574393515952034305194542857496045"
      "531676044756160413302774714984450425759043258192756735");  // 2^536 - 1

  Buffer out;
  ASSERT_TRUE(compact::encodeInteger(v, out));
  ASSERT_EQ(
      out.toVector(),
      "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
      "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"_unhex);
  // clang-format on
}

/**
 * @given a BigInteger value exceeding the range supported by scale
 * @when encode it a directly as BigInteger
 * @then obtain kValueIsTooBig error
 */
TEST(Scale, compactEncodeOutOfRangeBigInteger) {
  // try to encode out of range big integer value MAX_BIGINT + 1 == 2^536
  // too big value, even for big integer case
  // we are going to have kValueIsTooBig error
  BigInteger v(
      "224945689727159819140526925384299092943484855915095831"
      "655037778630591879033574393515952034305194542857496045"
      "531676044756160413302774714984450425759043258192756736");  // 2^536

  Buffer out;
  // value is too big, it is not encoded
  ASSERT_FALSE(compact::encodeInteger(v, out));
  ASSERT_EQ(out.size(), 0);  // nothing was written to buffer
}
