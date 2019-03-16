/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "common/result.hpp"
#include "common/scale/basic_stream.hpp"
#include "common/scale/compact.hpp"

using namespace kagome;
using namespace kagome::common;
using namespace common::scale;

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
  auto stream = BasicStream{bytes};
  auto result = compact::decodeInteger(stream);
  result.match(
      [](const expected::Value<BigInteger> &v) { ASSERT_EQ(v.value, 0); },
      [](const expected::Error<DecodeError> &e) { FAIL(); });
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
  auto stream = BasicStream{bytes};
  auto result = compact::decodeInteger(stream);
  result.match(
      [](const expected::Value<BigInteger> &v) { ASSERT_EQ(v.value, 1); },
      [](const expected::Error<DecodeError> &e) { FAIL(); });
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
  auto stream = BasicStream{bytes};
  auto result = compact::decodeInteger(stream);
  result.match(
      [](const expected::Value<BigInteger> &v) { ASSERT_EQ(v.value, 63); },
      [](const expected::Error<DecodeError> &e) { FAIL(); });
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
  auto stream = BasicStream{bytes};
  auto result = compact::decodeInteger(stream);
  result.match(
      [](const expected::Value<BigInteger> &v) { ASSERT_EQ(v.value, 64); },
      [](const expected::Error<DecodeError> &e) { FAIL(); });
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
  auto stream = BasicStream{bytes};
  auto result = compact::decodeInteger(stream);
  result.match(
      [](const expected::Value<BigInteger> &v) { ASSERT_EQ(v.value, 16383); },
      [](const expected::Error<DecodeError> &e) { FAIL(); });
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
  auto stream = BasicStream{bytes};
  auto result = compact::decodeInteger(stream);
  result.match(
      [](const expected::Value<BigInteger> &v) { ASSERT_EQ(v.value, 16384); },
      [](const expected::Error<DecodeError> &e) { FAIL(); });
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
  auto stream = BasicStream{bytes};
  auto result = compact::decodeInteger(stream);
  result.match(
      [](const expected::Value<BigInteger> &v) {
        ASSERT_EQ(v.value, 1073741823);
      },
      [](const expected::Error<DecodeError> &e) { FAIL(); });
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
  auto stream = BasicStream{bytes};
  auto result = compact::decodeInteger(stream);
  result.match(
      [](const expected::Value<BigInteger> &v) {
        ASSERT_EQ(v.value, 1073741824);
      },
      [](const expected::Error<DecodeError> &e) { FAIL(); });
}

/**
 * @given incorrect byte array, which assumes 4-th case of encoding
 * @when apply decodeInteger
 * @then get kNotEnoughData error
 */
TEST(Scale, compactDecodeBigIntegerError) {
  auto bytes = ByteArray{255, 255, 255, 255};
  auto stream = BasicStream{bytes};
  auto result = compact::decodeInteger(stream);
  result.match([](const expected::Value<BigInteger> &v) { FAIL(); },
               [](const expected::Error<DecodeError> &e) {
                 ASSERT_EQ(e.error, DecodeError::kNotEnoughData);
               });
}

/**
 * @given max value of first category integer = 2^6 - 1 = 63
 * @when encode it by compact::encodeInteger
 * @then obtain expected result: 1 byte representation
 */
TEST(Scale, compactEncodeFirstCategory) {
  // encode MAX_UI8 := 63
  compact::encodeInteger(63).match(
      [&](const expected::Value<ByteArray> &res) {
        ASSERT_EQ(res.value, std::vector<uint8_t>({252}));
      },
      [&](const expected::Error<EncodeError> &) { FAIL(); });
}

/**
 * @given several encoding cases which needs 2 byte representation
 * @when encode it a by compact::encodeInteger
 * @then obtain expected result: 2 bytes representation
 */
TEST(Scale, compactEncodeSecondCategory) {
  // encode MIN_UI16 := MAX_UI8 + 1
  compact::encodeInteger(64).match(
      [&](const expected::Value<ByteArray> &res) {
        ASSERT_EQ(res.value, std::vector<uint8_t>({1, 1}));
      },
      [&](const expected::Error<EncodeError> &) { FAIL(); });

  // encode some UI16
  compact::encodeInteger(255).match(
      [&](const expected::Value<ByteArray> &res) {
        ASSERT_EQ(res.value, std::vector<uint8_t>({253, 3}));
      },
      [&](const expected::Error<EncodeError> &) { FAIL(); });

  // encode some other UI16
  compact::encodeInteger(511).match(
      [&](const expected::Value<ByteArray> &res) {
        ASSERT_EQ(res.value, std::vector<uint8_t>({253, 7}));
      },
      [&](const expected::Error<EncodeError> &) { FAIL(); });

  // encode MAX_UI16 := 2^14 - 1 = 16383
  compact::encodeInteger(16383).match(
      [&](const expected::Value<ByteArray> &res) {
        ASSERT_EQ(res.value, std::vector<uint8_t>({253, 255}));
      },
      [&](const expected::Error<EncodeError> &) { FAIL(); });
}

/**
 * @given min, max and intermediate values of third category = {2^14, 2^16 - 1,
 * 2^30 - 1}
 * @when encode it a by compact::encodeInteger
 * @then obtain expected result: 4 bytes representation
 */
TEST(Scale, compactEncodeThirdCategory) {
  // encode MIN_UI32 := MAX_UI16 + 1 == 16384
  compact::encodeInteger(16384).match(
      [&](const expected::Value<ByteArray> &res) {
        ASSERT_EQ(res.value, std::vector<uint8_t>({2, 0, 1, 0}));
      },
      [&](const expected::Error<EncodeError> &) { FAIL(); });

  // encode some uint16_t value which requires 4 bytes
  compact::encodeInteger(65535).match(
      [&](const expected::Value<ByteArray> &res) {
        ASSERT_EQ(res.value, std::vector<uint8_t>({254, 255, 3, 0}));
      },
      [&](const expected::Error<EncodeError> &) { FAIL(); });

  // 2^30 - 1 is max 4 byte value
  // encode MAX_UI32 := 2^30 == 1073741823
  compact::encodeInteger(1073741823)
      .match(
          [](const expected::Value<ByteArray> &res) {
            ASSERT_EQ(res.value, std::vector<uint8_t>({254, 255, 255, 255}));
          },
          [](const expected::Error<EncodeError> &) { FAIL(); });
}

/**
 * @given max value of first category = 2^6 - 1 = 63
 * @when encode it a directly as BigInteger
 * @then obtain expected result: 1 byte representation
 */
TEST(Scale, compactEncodeFirstCategoryBigInteger) {
  BigInteger v("63");  // 2^6 - 1

  compact::encodeInteger(v).match(
      [](const expected::Value<ByteArray> &res) {
        ASSERT_EQ(res.value, ByteArray({252}));
      },
      [](const expected::Error<EncodeError> &) { FAIL(); });
}

/**
 * @given max value of second category = 2^14 - 1
 * @when encode it a directly as BigInteger
 * @then obtain expected result: 2 bytes representation
 */
TEST(Scale, compactEncodeSecondCategoryBigInteger) {
  BigInteger v("16383");  // 2^14 - 1

  compact::encodeInteger(v).match(
      [](const expected::Value<ByteArray> &res) {
        ASSERT_EQ(res.value, ByteArray({253, 255}));
      },
      [](const expected::Error<EncodeError> &) { FAIL(); });
}

/**
 * @given max value of third category = 2^30 - 1
 * @when encode it a directly as BigInteger
 * @then obtain expected result: 4 bytes representation
 */
TEST(Scale, compactEncodeThirdCategoryBigInteger) {
  BigInteger v("1073741823");  // 2^30 - 1

  compact::encodeInteger(v).match(
      [](const expected::Value<ByteArray> &res) {
        ASSERT_EQ(res.value, ByteArray({254, 255, 255, 255}));
      },
      [](const expected::Error<EncodeError> &) { FAIL(); });
}

/**
 * @given some value of fourth category
 * @when encode it a directly as BigInteger
 * @then obtain expected result: multibyte representation
 */
TEST(Scale, compactEncodeFourthCategoryBigInteger) {
  BigInteger v(
      "1234567890123456789012345678901234567890");  // (1234567890) x 4 times

  compact::encodeInteger(v).match(
      [](const expected::Value<ByteArray> &res) {
        ASSERT_EQ(res.value,
                  ByteArray(
                      // clang-format off
                      // header comes first, length is 17 => header = 55 == (17 - 4) * 4 + 0b11 :
                      {0b110111,
                       210, 10, 63, 206, 150, 95, 188, 172, 184, 243, 219, 192, 117, 32, 201, 160, 3}));
        // clang-format on
      },
      [](const expected::Error<EncodeError> &) { FAIL(); });
}

/**
 * @given min value which must be encoded as 4-th case
 * @when encode it a directly as BigInteger
 * @then obtain expected result: multibyte representation
 */
TEST(Scale, compactEncodeMinBigInteger) {
  BigInteger v(1073741824);
  compact::encodeInteger(v).match(
      [](const expected::Value<ByteArray> &res) {
        ASSERT_EQ(res.value,
                  std::vector<uint8_t>(
                      // clang-format off
                          // header
                          {0b00000011,
                           // value
                           0, 0, 0, 64}));
        // clang-format on
      },
      [](const expected::Error<EncodeError> &) { FAIL(); });
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

  compact::encodeInteger(v).match(
      [](const expected::Value<ByteArray> &res) {
        ASSERT_EQ(
            res.value,
            std::vector<uint8_t>(
                // clang-format off
                // header comes first, length is 67 => header = 255 == (67 - 4) * 4 + 0b11 :
                {0b11111111,
                 // now 67 bytes of 255 = 0xFF
                 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
                 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
                 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
                 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
                 255, 255, 255, 255, 255, 255, 255}));
        // clang-format on
      },
      [](const expected::Error<EncodeError> &) { FAIL(); });
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

  compact::encodeInteger(v).match(
      [](const expected::Value<ByteArray> &) { FAIL(); },
      [](const expected::Error<EncodeError> &e) {
        ASSERT_EQ(e.error, EncodeError::kValueIsTooBig);
      });
}
