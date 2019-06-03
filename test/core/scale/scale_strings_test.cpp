/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "scale/scale.hpp"
#include "testutil/outcome.hpp"

using kagome::scale::ByteArray;
using kagome::scale::ScaleDecoderStream;
using kagome::scale::ScaleEncoderStream;

/**
 * @given raw string
 * @when specified string is encoded by ScaleEncoderStream
 * @then encoded value meets expectations
 */
TEST(Scale, RawStringEncodeSuccess) {
auto * v = "asdadad";
ScaleEncoderStream s{};
ASSERT_NO_THROW((s << v));
ASSERT_EQ(s.data(), (ByteArray{28, 'a', 's', 'd', 'a', 'd', 'a', 'd'}));
}

/**
 * @given raw string
 * @when specified string is encoded by ScaleEncoderStream
 * @then encoded value meets expectations
 */
TEST(Scale, StdStringEncodeSuccess) {
std::string v = "asdadad";
ScaleEncoderStream s;
ASSERT_NO_THROW((s << v));
ASSERT_EQ(s.data(), (ByteArray{28, 'a', 's', 'd', 'a', 'd', 'a', 'd'}));
}

/**
 * @given byte array containing encoded string
 * @when string is decoded using ScaleDecoderStream
 * @then decoded string matches expectations
 */
TEST(Scale, StringDecodeSuccess) {
auto bytes = ByteArray{28, 'a', 's', 'd', 'a', 'd', 'a', 'd'};
ScaleDecoderStream s(bytes);
std::string v;
ASSERT_NO_THROW(s >> v);
ASSERT_EQ(v, "asdadad");
}
