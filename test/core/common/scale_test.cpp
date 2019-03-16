/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "common/result.hpp"
#include "common/scale.hpp"
#include "common/scale/basic_stream.hpp"

using namespace kagome;
using namespace kagome::common;
using namespace common::scale;

/**
 * Utility tests
 */

/**
 * @given byte array of 3 items: 0, 1, 2
 * @when create BasicStream wrapping this array and start to get bytes one by
 * one
 * @then bytes 0, 1, 2 are obtained sequencially
 */
TEST(Scale, basicStream) {
  auto bytes = ByteArray{0, 1, 2};
  auto stream = BasicStream{bytes};
  ASSERT_EQ(stream.hasMore(3), true);

  auto byte = stream.nextByte();
  ASSERT_EQ(byte.has_value(), true);
  ASSERT_EQ((*byte), 0);

  ASSERT_EQ(stream.hasMore(2), true);
  byte = stream.nextByte();
  ASSERT_EQ(byte.has_value(), true);
  ASSERT_EQ((*byte), 1);

  ASSERT_EQ(stream.hasMore(1), true);
  byte = stream.nextByte();
  ASSERT_EQ(byte.has_value(), true);
  ASSERT_EQ((*byte), 2);

  ASSERT_EQ(stream.hasMore(1), false);
}

/**
 * Decode compact integers tests
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
        ASSERT_EQ(res.value,
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

/**
 * Fixedwidth methods tests
 */

/**
 * @given bool values: true and false
 * @when encode them by fixedwidth::encodeBool function
 * @then obtain expected result each time
 */
TEST(Scale, fixedwidthEncodeBool) {
  ASSERT_EQ(boolean::encodeBool(true), (ByteArray{0x1}));
  ASSERT_EQ(boolean::encodeBool(false), (ByteArray{0x0}));
}

/**
 * @given byte array containing values {0, 1, 2}
 * @when fixedwidth::decodeBool function is applied sequentially
 * @then it returns false, true and kUnexpectedValue error correspondingly
 */
TEST(Scale, fixedwidthDecodeBool) {
  //  fixedwidth::DecodeBoolRes
  // decode false
  auto bytes = ByteArray{0x0, 0x1, 0x2};
  auto stream = BasicStream{bytes};
  boolean::decodeBool(stream).match(
      [](const boolean::DecodeBoolResult::ValueType &v) {
        ASSERT_EQ(v.value, false);
      },
      [](const boolean::DecodeBoolResult::ErrorType &) { FAIL(); });

  // decode true
  boolean::decodeBool(stream).match(
      [](const boolean::DecodeBoolResult::ValueType &v) {
        ASSERT_EQ(v.value, true);
      },
      [](const boolean::DecodeBoolResult::ErrorType &) { FAIL(); });

  // decode unexpected value, we are going to have kUnexpectedValue error
  boolean::decodeBool(stream).match(
      [](const boolean::DecodeBoolResult::ValueType &v) { FAIL(); },
      [](const boolean::DecodeBoolResult::ErrorType &e) {
        ASSERT_EQ(e.error, DecodeError::kUnexpectedValue);
      });
}

/**
 * @given tribool values false, true and indeterminate
 * @when fixedwidth::encodeTribool function is applied sequentially
 * @then it returns 0, 1 and 2 correspondingly
 */
TEST(Scale, fixedwidthEncodeTribool) {
  // encode none
  ASSERT_EQ(0x0, boolean::encodeTribool(false));

  // encode false
  ASSERT_EQ(0x1, boolean::encodeTribool(true));

  // encode true
  ASSERT_EQ(0x2, boolean::encodeTribool(indeterminate));
}

/**
 * @given byte array {0, 1, 2, 3}
 * @when decodeTribool function is applied sequentially
 * @then it returns false, true, indeterminate and kUnexpectedValue error as
 * expected
 */
TEST(Scale, fixedwidthDecodeTribool) {
  // decode none
  auto bytes = ByteArray{0x0, 0x1, 0x2, 0x3};
  auto stream = BasicStream{bytes};
  boolean::decodeTribool(stream).match(
      [](const boolean::DecodeTriboolResult::ValueType &v) {
        ASSERT_EQ(false, v.value);
      },
      [](const boolean::DecodeTriboolResult::ErrorType &e) { FAIL(); });

  // decode false
  boolean::decodeTribool(stream).match(
      [](const boolean::DecodeTriboolResult::ValueType &v) {
        ASSERT_EQ(true, v.value);
      },
      [](const boolean::DecodeTriboolResult::ErrorType &e) { FAIL(); });

  // decode true
  boolean::decodeTribool(stream).match(
      [](const boolean::DecodeTriboolResult::ValueType &v) {
        ASSERT_EQ(isIndeterminate(v.value), true);
      },
      [](const boolean::DecodeTriboolResult::ErrorType &e) { FAIL(); });

  // decode unexpected value
  boolean::decodeTribool(stream).match(
      [](const boolean::DecodeTriboolResult::ValueType &v) { FAIL(); },
      [](const boolean::DecodeTriboolResult::ErrorType &e) {
        ASSERT_EQ(DecodeError::kUnexpectedValue, e.error);
      });
}

/**
 * @given variety of integer numbers of different types
 * @when suitable encode function is applied
 * @then expected result obtained
 */
TEST(Scale, fixedwidthEncodeIntegers) {
  // encode int8_t
  ASSERT_EQ(fixedwidth::encodeInt8(0), (ByteArray{0}));
  ASSERT_EQ(fixedwidth::encodeInt8(-1), (ByteArray{255}));
  ASSERT_EQ(fixedwidth::encodeInt8(-128), (ByteArray{128}));
  ASSERT_EQ(fixedwidth::encodeInt8(-127), (ByteArray{129}));
  ASSERT_EQ(fixedwidth::encodeInt8(123), (ByteArray{123}));
  ASSERT_EQ(fixedwidth::encodeInt8(-15), (ByteArray{241}));

  // encode uint8_t
  ASSERT_EQ(fixedwidth::encodeUInt8(0), (ByteArray{0}));
  ASSERT_EQ(fixedwidth::encodeUInt8(234), (ByteArray{234}));
  ASSERT_EQ(fixedwidth::encodeUInt8(255), (ByteArray{255}));

  // encode int16_t
  ASSERT_EQ(fixedwidth::encodeInt16(-32767), (ByteArray{1, 128}));
  ASSERT_EQ(fixedwidth::encodeInt16(-32768), (ByteArray{0, 128}));
  ASSERT_EQ(fixedwidth::encodeInt16(-1), (ByteArray{255, 255}));
  ASSERT_EQ(fixedwidth::encodeInt16(32767), (ByteArray{255, 127}));

  // encode uint16_t
  ASSERT_EQ(fixedwidth::encodeUint16(32770), (ByteArray{2, 128}));

  // encode int32_t
  ASSERT_EQ(fixedwidth::encodeInt32(2147483647),
            (ByteArray{255, 255, 255, 127}));  // max positive int32_t
  ASSERT_EQ(fixedwidth::encodeInt32(-1), (ByteArray{255, 255, 255, 255}));

  // encode uint32_t
  ASSERT_EQ(fixedwidth::encodeUint32(16909060), (ByteArray{4, 3, 2, 1}));
  ASSERT_EQ(fixedwidth::encodeUint32(67305985), (ByteArray{1, 2, 3, 4}));

  // encode int64_t
  ASSERT_EQ(fixedwidth::encodeInt64(578437695752307201),
            (ByteArray{1, 2, 3, 4, 5, 6, 7, 8}));
  ASSERT_EQ(fixedwidth::encodeInt64(-1),
            (ByteArray{255, 255, 255, 255, 255, 255, 255, 255}));

  // encode uint64_t
  ASSERT_EQ(fixedwidth::encodeUint64(578437695752307201),
            (ByteArray{1, 2, 3, 4, 5, 6, 7, 8}));
}

/**
 * @given byte array containing encoded int8_t values
 * @when fixedwidth::decodeInt8 is applied
 * @then correct sequence of decoded values is obtained
 */
TEST(Scale, fixedwidthDecodeInt8) {
  // decode int8_t
  auto bytes = ByteArray{0, 255, 128, 129, 123, 241};
  auto stream = BasicStream{bytes};

  {
    auto val = fixedwidth::decodeInt8(stream);
    ASSERT_EQ(val.has_value(), true);
    ASSERT_EQ((*val), 0);
  }
  {
    auto val = fixedwidth::decodeInt8(stream);
    ASSERT_EQ(val.has_value(), true);
    ASSERT_EQ((*val), -1);
  }
  {
    auto val = fixedwidth::decodeInt8(stream);
    ASSERT_EQ(val.has_value(), true);
    ASSERT_EQ((*val), -128);
  }
  {
    auto val = fixedwidth::decodeInt8(stream);
    ASSERT_EQ(val.has_value(), true);
    ASSERT_EQ((*val), -127);
  }
  {
    auto val = fixedwidth::decodeInt8(stream);
    ASSERT_EQ(val.has_value(), true);
    ASSERT_EQ((*val), 123);
  }
  {
    auto val = fixedwidth::decodeInt8(stream);
    ASSERT_EQ(val.has_value(), true);
    ASSERT_EQ((*val), -15);
  }
}

/**
 * @given byte array containing encoded uint8_t values
 * @when fixedwidth::decodeUint8 is applied
 * @then correct sequence of decoded values is obtained
 */
TEST(Scale, fixedwidthDecodeUint8) {
  // decode uint8_t
  auto bytes = ByteArray{0, 234, 255};
  auto stream = BasicStream{bytes};

  {
    auto val = fixedwidth::decodeUint8(stream);
    ASSERT_EQ(val.has_value(), true);
    ASSERT_EQ((*val), 0);
  }
  {
    auto val = fixedwidth::decodeUint8(stream);
    ASSERT_EQ(val.has_value(), true);
    ASSERT_EQ((*val), 234);
  }
  {
    auto val = fixedwidth::decodeUint8(stream);
    ASSERT_EQ(val.has_value(), true);
    ASSERT_EQ((*val), 255);
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
  auto bytes = ByteArray{1, 128,
                         0, 128,
                         255, 255,
                         255, 127};
  // clang-format on

  auto stream = BasicStream{bytes};

  {
    auto val = fixedwidth::decodeInt16(stream);
    ASSERT_EQ(val.has_value(), true);
    ASSERT_EQ((*val), -32767);
  }
  {
    auto val = fixedwidth::decodeInt16(stream);
    ASSERT_EQ(val.has_value(), true);
    ASSERT_EQ((*val), -32768);
  }
  {
    auto val = fixedwidth::decodeInt16(stream);
    ASSERT_EQ(val.has_value(), true);
    ASSERT_EQ((*val), -1);
  }
  {
    auto val = fixedwidth::decodeInt16(stream);
    ASSERT_EQ(val.has_value(), true);
    ASSERT_EQ((*val), 32767);
  }
}

/**
 * @given byte array containing encoded uint16_t values
 * @when fixedwidth::decodeUint16 is applied
 * @then correct sequence of decoded values is obtained
 */
TEST(Scale, fixedwidthDecodeUint16) {
  // decode uint16_t
  auto bytes = ByteArray{2, 128};

  auto stream = BasicStream{bytes};

  {
    auto val = fixedwidth::decodeUint16(stream);
    ASSERT_EQ(val.has_value(), true);
    ASSERT_EQ((*val), 32770);
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
  auto bytes = ByteArray{255, 255, 255, 127,
                         255, 255, 255, 255};
  //clang-format on

  auto stream = BasicStream{bytes};

  {
    auto val = fixedwidth::decodeInt32(stream);
    ASSERT_EQ(val.has_value(), true);
    ASSERT_EQ((*val), 2147483647);
  }
  {
    auto val = fixedwidth::decodeInt32(stream);
    ASSERT_EQ(val.has_value(), true);
    ASSERT_EQ((*val), -1);
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
  auto bytes = ByteArray{4, 3, 2, 1,
                         1, 2, 3, 4};
  //clang-format on

  auto stream = BasicStream{bytes};

  {
    auto val = fixedwidth::decodeUint32(stream);
    ASSERT_EQ(val.has_value(), true);
    ASSERT_EQ((*val), 16909060);
  }
  {
    auto val = fixedwidth::decodeUint32(stream);
    ASSERT_EQ(val.has_value(), true);
    ASSERT_EQ((*val), 67305985);
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
  auto bytes = ByteArray{1, 2, 3, 4, 5, 6, 7, 8,
                         255, 255, 255, 255, 255, 255, 255, 255};
  //clang-format on

  auto stream = BasicStream{bytes};

  {
    auto val = fixedwidth::decodeInt64(stream);
    ASSERT_EQ(val.has_value(), true);
    ASSERT_EQ((*val), 578437695752307201);
  }
  {
    auto val = fixedwidth::decodeInt64(stream);
    ASSERT_EQ(val.has_value(), true);
    ASSERT_EQ((*val), -1);
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
  auto bytes = ByteArray{1, 2, 3, 4, 5, 6, 7, 8,
                         255, 255, 255, 255, 255, 255, 255, 255};
  //clang-format on

  auto stream = BasicStream{bytes};

  {
    auto val = fixedwidth::decodeUint64(stream);
    ASSERT_EQ(val.has_value(), true);
    ASSERT_EQ((*val), 578437695752307201);
  }
  {
    auto val = fixedwidth::decodeUint64(stream);
    ASSERT_EQ(val.has_value(), true);
    ASSERT_EQ((*val), 18446744073709551615ull);
  }
}

/**
 * @given collection of 80 items of type uint8_t
 * @when encodeCollection is applied
 * @then expected result is obtained: header is 2 byte, items are 1 byte each
 */
TEST(Scale, encodeCollectionOf80) {
  // 80 items of value 1
  std::vector<uint8_t> collection = {
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  };
  auto res = collection::encodeCollection(collection);
  res.match(
      [&collection](const EncodeResult::ValueType &v) {
        // clang-format off
          ASSERT_EQ(v.value,
                    (ByteArray{65, 1, // header: 80 > 63 so 2 bytes required
                               // (80 << 2) + 1 = 321 = 65 + 256 * 1
                               1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                               1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                               1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                               1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                    }));
        // clang-format on
      },
      [](const EncodeResult::ErrorType &e) { FAIL(); });
}

/**
 * @given collection of items of type uint16_t
 * @when encodeCollection is applied
 * @then expected result is obtained
 */
TEST(Scale, encodeCollectionUint16) {
  std::vector<uint16_t> collection = {1, 2, 3, 4};
  auto res = collection::encodeCollection(collection);
  res.match(
      [&collection](const EncodeResult::ValueType &v) {
        // clang-format off
        ASSERT_EQ(v.value,
                  (ByteArray{
                      16,  // header
                      1, 0,  // first item
                      2, 0,  // second item
                      3, 0,  // third item
                      4, 0  // fourth item
                  }));
        // clang-format on
      },
      [](const EncodeResult::ErrorType &e) { FAIL(); });
}
// check the same for 4 and 8 bytes

/**
 * @given collection of items of type uint16_t containing 2^14 items
 * where collection[i]  == i % 256
 * @when encodeCollection is applied
 * @then obtain byte array of length 32772 bytes
 * where each second byte == 0 and collection[(i-4)/2] == (i/2) % 256
 */
TEST(Scale, encodeLongCollectionUint16) {
  std::vector<uint16_t> collection;
  auto length = 16384;
  collection.reserve(length);
  for (auto i = 0; i < length; ++i) {
    collection.push_back(i % 256);
  }

  auto res = collection::encodeCollection(collection);
  res.match(
      [&collection, length](const EncodeResult::ValueType &v) {
        ASSERT_EQ(v.value.size(), (length * 2 + 4));
        // header takes 4 byte,
        // first 4 bytes represent le-encoded value 2^16 + 2
        // which is compact-encoded value 2^14 = 16384
        auto stream = BasicStream(v.value);

        compact::decodeInteger(stream).match(
            [](const compact::DecodeIntegerResult::ValueType &bi) {
              ASSERT_EQ(bi.value, 16384);
            },
            [](const compact::DecodeIntegerResult::ErrorType &) { FAIL(); });
        // now only 32768 bytes left in stream
        ASSERT_EQ(stream.hasMore(32768), true);
        ASSERT_EQ(stream.hasMore(32769), false);

        for (auto i = 0; i < length; ++i) {
          auto byte = stream.nextByte();
          ASSERT_EQ(byte.has_value(), true);
          ASSERT_EQ((*byte), i % 256);
          byte = stream.nextByte();
          ASSERT_EQ(byte.has_value(), true);
          ASSERT_EQ((*byte), 0);
        }

        ASSERT_EQ(stream.hasMore(1), false);
      },
      [](const EncodeResult::ErrorType &e) { FAIL(); });
}

/**
 * @given very long collection of items of type uint8_t containing 2^20 items
 * this number takes ~ 1 Mb of data
 * where collection[i]  == i % 256
 * @when encodeCollection is applied
 * @then obtain byte array of length 1048576 + 4 bytes (header) bytes
 * where first bytes repreent header, other are data itself
 * where each byte after header == i%256
 */

TEST(Scale, encodeVeryLongCollectionUint8) {
  std::vector<uint8_t> collection;
  auto length = 1048576;  // 2^20
  collection.reserve(length);
  for (auto i = 0; i < length; ++i) {
    collection.push_back(i % 256);
  }

  auto res = collection::encodeCollection(collection);
  res.match(
      [&collection, length](const EncodeResult::ValueType &v) {
        ASSERT_EQ(v.value.size(), (length + 4));
        // header takes 4 bytes,
        // first byte == (4-4) + 3 = 3, which means that number of items
        // requires 4 bytes
        // 3 next bytes are 0, and the last 4-th == 2^6 == 64
        // which is compact-encoded value 2^14 = 16384
        auto stream = BasicStream(v.value);

        compact::decodeInteger(stream).match(
            [](const compact::DecodeIntegerResult::ValueType &bi) {
              ASSERT_EQ(bi.value, 1048576);
            },
            [](const compact::DecodeIntegerResult::ErrorType &) { FAIL(); });
        // now only 1048576 bytes left in stream
        ASSERT_EQ(stream.hasMore(1048576), true);
        ASSERT_EQ(stream.hasMore(1048576 + 1), false);

        for (auto i = 0; i < length; ++i) {
          auto byte = stream.nextByte();
          ASSERT_EQ(byte.has_value(), true);
          ASSERT_EQ((*byte), i % 256);
        }

        ASSERT_EQ(stream.hasMore(1), false);
      },
      [](const EncodeResult::ErrorType &e) { FAIL(); });
}

// following test takes too much time, don't run it
/**
 * @given very long collection of items of type uint8_t containing 2^30 ==
 * 1073741824 items this number takes ~ 1 Gb of data where collection[i]  == i %
 * 256
 * @when encodeCollection is applied
 * @then obtain byte array of length 1073741824 + 5 bytes (header) bytes
 * where first bytes represent header, other are data itself
 * where each byte after header == i%256
 */
TEST(Scale, DISABLED_encodeVeryLongCollectionUint8) {
  std::vector<uint8_t> collection;
  auto length = 1073741824;  // 2^20
  collection.reserve(length);
  for (auto i = 0; i < length; ++i) {
    collection.push_back(i % 256);
  }

  auto res = collection::encodeCollection(collection);
  res.match(
      [&collection, length](const EncodeResult::ValueType &v) {
        ASSERT_EQ(v.value.size(), (length + 4));
        // header takes 4 bytes,
        // first byte == (4-4) + 3 = 3, which means that number of items
        // requires 4 bytes
        // 3 next bytes are 0, and the last 4-th == 2^6 == 64
        // which is compact-encoded value 2^14 = 16384
        auto stream = BasicStream(v.value);

        compact::decodeInteger(stream).match(
            [length](const compact::DecodeIntegerResult::ValueType &bi) {
              ASSERT_EQ(bi.value, length);
            },
            [](const compact::DecodeIntegerResult::ErrorType &) { FAIL(); });
        // now only 1048576 bytes left in stream
        ASSERT_EQ(stream.hasMore(length), true);
        ASSERT_EQ(stream.hasMore(length + 1), false);

        for (auto i = 0; i < length; ++i) {
          auto byte = stream.nextByte();
          ASSERT_EQ(byte.has_value(), true);
          ASSERT_EQ((*byte), i % 256);
        }

        ASSERT_EQ(stream.hasMore(1), false);
      },
      [](const EncodeResult::ErrorType &e) { FAIL(); });
}

/**
 * @given byte array representing encoded collection of
 * 4 uint16_t numbers {1, 2, 3, 4}
 * @when decodeCollection is applied
 * @then decoded collection {1, 2, 3, 4} is obtained
 */
TEST(Scale, decodeSimpleCollectionOfUint16) {
  std::vector<uint16_t> collection = {1, 2, 3, 4};
  // clang-format off
  auto bytes = ByteArray{
      16,   // header
      1, 0, // first item
      2, 0, // second item
      3, 0, // third item
      4, 0  // fourth item
  };
  // clang-format on
  auto stream = BasicStream{bytes};
  auto res = collection::decodeCollection<uint16_t>(stream);
  res.match(
      [&collection](const expected::Value<std::vector<uint16_t>> &v) {
        ASSERT_EQ(v.value.size(), 4);
        ASSERT_EQ(v.value, collection);
      },
      [](const expected::Error<DecodeError> &e) { FAIL(); });
}

/**
 * @given encoded long collection ~ 1 Mb of data
 * @when apply decodeCollection
 * @then obtain source collection
 */
TEST(Scale, decodeLongCollectionOfUint8) {
  std::vector<uint8_t> collection;
  auto length = 1048576;  // 2^20
  collection.reserve(length);
  for (auto i = 0; i < length; ++i) {
    collection.push_back(i % 256);
  }

  auto res = collection::encodeCollection(collection);
  res.match(
      [&collection, length](const EncodeResult::ValueType &v) {
        auto stream = BasicStream(v.value);
        auto decodeResult = collection::decodeCollection<uint8_t>(stream);
        decodeResult.match(
            [&collection](
                collection::DecodeCollectionResult<uint8_t>::ValueType &val) {
              ASSERT_EQ(val.value, collection);
            },
            [](expected::Error<DecodeError> &) { FAIL(); });
      },
      [](const EncodeResult::ErrorType &e) { FAIL(); });
}

TEST(Scale, encodeOptional) {
  // most simple case
  optional::encodeOptional(std::optional<uint8_t>{std::nullopt})
      .match(
          [](const expected::Value<ByteArray> &v) {
            ASSERT_EQ(v.value, (ByteArray{0}));
          },
          [](const expected::Error<EncodeError> &) { FAIL(); });

  // encode existing uint8_t
  optional::encodeOptional(std::optional<uint8_t>{1})
      .match(
          [](const expected::Value<ByteArray> &v) {
            ASSERT_EQ(v.value, (ByteArray{1, 1}));
          },
          [](const expected::Error<EncodeError> &) { FAIL(); });
  // encode negative int8_t
  optional::encodeOptional(std::optional<int8_t>{-1})
      .match(
          [](const expected::Value<ByteArray> &v) {
            ASSERT_EQ(v.value, (ByteArray{1, 255}));
          },
          [](const expected::Error<EncodeError> &) { FAIL(); });

  // encode non-existing uint16_t
  optional::encodeOptional<uint16_t>(std::nullopt)
      .match(
          [](const expected::Value<ByteArray> &v) {
            ASSERT_EQ(v.value, (ByteArray{0}));
          },
          [](const expected::Error<EncodeError> &) { FAIL(); });

  // encode existing uint16_t
  optional::encodeOptional(std::optional<uint16_t>{511})
      .match(
          [](const expected::Value<ByteArray> &v) {
            ASSERT_EQ(v.value, (ByteArray{1, 255, 1}));
          },
          [](const expected::Error<EncodeError> &) { FAIL(); });

  // encode existing uint32_t
  optional::encodeOptional(std::optional<uint32_t>{67305985})
      .match(
          [](const expected::Value<ByteArray> &v) {
            ASSERT_EQ(v.value, (ByteArray{1, 1, 2, 3, 4}));
          },
          [](const expected::Error<EncodeError> &) { FAIL(); });
}

TEST(Scale, decodeOptional) {
  // clang-format off
  auto bytes = ByteArray{
    0,              // first value
    1, 1,           // second value
    1, 255,         // third value
    0,              // fourth value
    1, 255, 1,      // fifth value
    1, 1, 2, 3, 4}; // sixth value
  // clang-format on

  auto stream = BasicStream{bytes};

  // decode nullopt uint8_t
  optional::decodeOptional<uint8_t>(stream).match(
      [](const expected::Value<std::optional<uint8_t>> &v) {
        ASSERT_EQ(v.value, std::nullopt);
      },
      [](const expected::Error<DecodeError> &) { FAIL(); });

  // decode optional uint8_t
  optional::decodeOptional<uint8_t>(stream).match(
      [](const expected::Value<std::optional<uint8_t>> &v) {
        ASSERT_EQ(v.value.has_value(), true);
        ASSERT_EQ((*v.value), 1);
      },
      [](const expected::Error<DecodeError> &) { FAIL(); });

  // decode optional negative int8_t
  optional::decodeOptional<int8_t>(stream).match(
      [](const expected::Value<std::optional<int8_t>> &v) {
        ASSERT_EQ(v.value.has_value(), true);
        ASSERT_EQ((*v.value), -1);
      },
      [](const expected::Error<DecodeError> &) { FAIL(); });

  // decode nullopt uint16_t
  // it requires 1 zero byte just like any other nullopt
  optional::decodeOptional<uint16_t>(stream).match(
      [](const expected::Value<std::optional<uint16_t>> &v) {
        ASSERT_EQ(v.value.has_value(), false);
      },
      [](const expected::Error<DecodeError> &) { FAIL(); });

  // decode optional uint16_t
  optional::decodeOptional<uint16_t>(stream).match(
      [](const expected::Value<std::optional<uint16_t>> &v) {
        ASSERT_EQ(v.value.has_value(), true);
        ASSERT_EQ((*v.value), 511);
      },
      [](const expected::Error<DecodeError> &) { FAIL(); });

  // decode optional uint32_t
  optional::decodeOptional<uint32_t>(stream).match(
      [](const expected::Value<std::optional<uint32_t>> &v) {
        ASSERT_EQ(v.value.has_value(), true);
        ASSERT_EQ((*v.value), 67305985);
      },
      [](const expected::Error<DecodeError> &) { FAIL(); });
}

// TODO: implement encode and decode tuples and variants

// TODO: write example of encoding and decoding custom structures

TEST(Scale, DISABLED_encodeTuple) {
  FAIL();
}

TEST(Scale, DISABLED_decodeTuple) {
  FAIL();
}

TEST(Scale, DISABLED_encodeStructureExample) {
  FAIL();
}

TEST(Scale, DISABLED_decodeStructureExample) {
  FAIL();
}

TEST(Scale, DISABLED_encodeVariant) {
  FAIL();
}

TEST(Scale, DISABLED_decodeVariant) {
  FAIL();
}
