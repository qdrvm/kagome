/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "common/result.hpp"
#include "common/scale/basic_stream.hpp"
#include "common/scale/fixedwidth.hpp"

using namespace kagome;
using namespace kagome::common;
using namespace common::scale;

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
    ASSERT_EQ(out.toVector(), (ByteArray{0}));
  }
  {
    Buffer out;
    fixedwidth::encodeInt8(-1, out);
    ASSERT_EQ(out.toVector(), (ByteArray{255}));
  }
  {
    Buffer out;
    fixedwidth::encodeInt8(-128, out);
    ASSERT_EQ(out.toVector(), (ByteArray{128}));
  }
  {
    Buffer out;
    fixedwidth::encodeInt8(-127, out);
    ASSERT_EQ(out.toVector(), (ByteArray{129}));
  }
  {
    Buffer out;
    fixedwidth::encodeInt8(123, out);
    ASSERT_EQ(out.toVector(), (ByteArray{123}));
  }
  {
    Buffer out;
    fixedwidth::encodeInt8(-15, out);
    ASSERT_EQ(out.toVector(), (ByteArray{241}));
  }
  // encode uint8_t
  {
    Buffer out;
    fixedwidth::encodeUInt8(0, out);
    ASSERT_EQ(out.toVector(), (ByteArray{0}));
  }
  {
    Buffer out;
    fixedwidth::encodeUInt8(234, out);
    ASSERT_EQ(out.toVector(), (ByteArray{234}));
  }
  {
    Buffer out;
    fixedwidth::encodeUInt8(255, out);
    ASSERT_EQ(out.toVector(), (ByteArray{255}));
  }
  // encode int16_t
  {
    Buffer out;
    fixedwidth::encodeInt16(-32767, out);
    ASSERT_EQ(out.toVector(), (ByteArray{1, 128}));
  }
  {
    Buffer out;
    fixedwidth::encodeInt16(-32768, out);
    ASSERT_EQ(out.toVector(), (ByteArray{0, 128}));
  }
  {
    Buffer out;
    fixedwidth::encodeInt16(-1, out);
    ASSERT_EQ(out.toVector(), (ByteArray{255, 255}));
  }
  {
    Buffer out;
    fixedwidth::encodeInt16(32767, out);
    ASSERT_EQ(out.toVector(), (ByteArray{255, 127}));
  }
  // encode uint16_t
  {
    Buffer out;
    fixedwidth::encodeUint16(32770, out);
    ASSERT_EQ(out.toVector(), (ByteArray{2, 128}));
  }
  // encode int32_t
  {
    Buffer out;
    fixedwidth::encodeInt32(2147483647, out);  // max positive int32_t
    ASSERT_EQ(out.toVector(), (ByteArray{255, 255, 255, 127}));
  }
  {
    Buffer out;
    fixedwidth::encodeInt32(-1, out);
    ASSERT_EQ(out.toVector(), (ByteArray{255, 255, 255, 255}));
  }
  // encode uint32_t
  {
    Buffer out;
    fixedwidth::encodeUint32(16909060, out);
    ASSERT_EQ(out.toVector(), (ByteArray{4, 3, 2, 1}));
  }
  {
    Buffer out;
    fixedwidth::encodeUint32(67305985, out);
    ASSERT_EQ(out.toVector(), (ByteArray{1, 2, 3, 4}));
  }
  // encode int64_t
  {
    Buffer out;
    fixedwidth::encodeInt64(578437695752307201ll, out);
    ASSERT_EQ(out.toVector(), (ByteArray{1, 2, 3, 4, 5, 6, 7, 8}));
  }
  {
    Buffer out;
    fixedwidth::encodeInt64(-1, out);
    ASSERT_EQ(out.toVector(),
              (ByteArray{255, 255, 255, 255, 255, 255, 255, 255}));
  }
  // encode uint64_t
  {
    Buffer out;
    fixedwidth::encodeUint64(578437695752307201ull, out);
    ASSERT_EQ(out.toVector(), (ByteArray{1, 2, 3, 4, 5, 6, 7, 8}));
  }
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
