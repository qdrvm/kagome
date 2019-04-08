/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "common/result.hpp"
#include "scale/byte_array_stream.hpp"
#include "scale/fixedwidth.hpp"

using namespace kagome;          // NOLINT
using namespace kagome::common;  // NOLINT
using namespace common::scale;   // NOLINT

/**
 * @given variety of integer numbers of different types
 * @when suitable encode function is applied
 * @then expected result obtained
 */
TEST(Scale, fixedwidthEncodeIntegers) {
  // encode int8_t
  {
    Buffer out;
    fixedwidth::encodeInt8(0, out);
    ASSERT_EQ(out, (Buffer{0}));
  }
  {
    Buffer out;
    fixedwidth::encodeInt8(-1, out);
    ASSERT_EQ(out, (Buffer{255}));
  }
  {
    Buffer out;
    fixedwidth::encodeInt8(-128, out);
    ASSERT_EQ(out, (Buffer{128}));
  }
  {
    Buffer out;
    fixedwidth::encodeInt8(-127, out);
    ASSERT_EQ(out, (Buffer{129}));
  }
  {
    Buffer out;
    fixedwidth::encodeInt8(123, out);
    ASSERT_EQ(out, (Buffer{123}));
  }
  {
    Buffer out;
    fixedwidth::encodeInt8(-15, out);
    ASSERT_EQ(out, (Buffer{241}));
  }
  // encode uint8_t
  {
    Buffer out;
    fixedwidth::encodeUInt8(0, out);
    ASSERT_EQ(out, (Buffer{0}));
  }
  {
    Buffer out;
    fixedwidth::encodeUInt8(234, out);
    ASSERT_EQ(out, (Buffer{234}));
  }
  {
    Buffer out;
    fixedwidth::encodeUInt8(255, out);
    ASSERT_EQ(out, (Buffer{255}));
  }
  // encode int16_t
  {
    Buffer out;
    fixedwidth::encodeInt16(-32767, out);
    ASSERT_EQ(out, (Buffer{1, 128}));
  }
  {
    Buffer out;
    fixedwidth::encodeInt16(-32768, out);
    ASSERT_EQ(out, (Buffer{0, 128}));
  }
  {
    Buffer out;
    fixedwidth::encodeInt16(-1, out);
    ASSERT_EQ(out, (Buffer{255, 255}));
  }
  {
    Buffer out;
    fixedwidth::encodeInt16(32767, out);
    ASSERT_EQ(out, (Buffer{255, 127}));
  }
  {
    Buffer out;
    fixedwidth::encodeInt16(12345, out);
    ASSERT_EQ(out, (Buffer{57, 48}));
  }
  {
    Buffer out;
    fixedwidth::encodeInt16(-12345, out);
    ASSERT_EQ(out, (Buffer{199, 207}));
  }
  // encode uint16_t
  {
    Buffer out;
    fixedwidth::encodeUint16(32770, out);
    ASSERT_EQ(out, (Buffer{2, 128}));
  }
  // encode int32_t
  {
    Buffer out;
    fixedwidth::encodeInt32(2147483647, out);  // max positive int32_t
    ASSERT_EQ(out, (Buffer{255, 255, 255, 127}));
  }
  {
    Buffer out;
    fixedwidth::encodeInt32(-1, out);
    ASSERT_EQ(out, (Buffer{255, 255, 255, 255}));
  }
  // encode uint32_t
  {
    Buffer out;
    fixedwidth::encodeUint32(16909060, out);
    ASSERT_EQ(out, (Buffer{4, 3, 2, 1}));
  }
  {
    Buffer out;
    fixedwidth::encodeUint32(67305985, out);
    ASSERT_EQ(out, (Buffer{1, 2, 3, 4}));
  }
  // encode int64_t
  {
    Buffer out;
    fixedwidth::encodeInt64(578437695752307201ll, out);
    ASSERT_EQ(out, (Buffer{1, 2, 3, 4, 5, 6, 7, 8}));
  }
  {
    Buffer out;
    fixedwidth::encodeInt64(-1, out);
    ASSERT_EQ(out, (Buffer{255, 255, 255, 255, 255, 255, 255, 255}));
  }
  // encode uint64_t
  {
    Buffer out;
    fixedwidth::encodeUint64(578437695752307201ull, out);
    ASSERT_EQ(out, (Buffer{1, 2, 3, 4, 5, 6, 7, 8}));
  }
}

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
