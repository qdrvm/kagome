/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include "scale/byte_array_stream.hpp"
#include "scale/type_decoder.hpp"
#include "scale/scale_encoder_stream.hpp"

using kagome::common::Buffer;
using kagome::common::ByteStream;
using kagome::scale::ByteArray;
using kagome::scale::ByteArrayStream;
using kagome::scale::ScaleEncoderStream;

/**
 * @given pair of values of different types: uint8_t and uint32_t
 * @when encode is applied
 * @then obtained serialized value meets predefined one
 */
TEST(Scale, encodePair) {
  uint8_t v1 = 1;
  uint32_t v2 = 1;

  ScaleEncoderStream s;
  ASSERT_NO_THROW((s << std::make_pair(v1, v2)));
  ASSERT_EQ(s.data(), (ByteArray{1, 1, 0, 0, 0}));
}

/**
 * @given byte sequence containign 2 encoded values of
 * different types: uint8_t and uint32_t
 * @when decode is applied
 * @then obtained pair mathces predefined one
 */
TEST(Scale, decodePair) {
  ByteArray bytes = {1, 1, 0, 0, 0};
  auto stream = ByteArrayStream{bytes};
  using Pair = std::pair<uint8_t, uint32_t>;
  kagome::scale::TypeDecoder<Pair> decoder;
  auto &&res = decoder.decode(stream);
  ASSERT_TRUE(res);
  ASSERT_EQ(res.value().first, 1);
  ASSERT_EQ(res.value().second, 1);
}
