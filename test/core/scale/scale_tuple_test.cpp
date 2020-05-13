/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "scale/scale.hpp"

#include <gtest/gtest.h>

using kagome::scale::ByteArray;
using kagome::scale::ScaleDecoderStream;
using kagome::scale::ScaleEncoderStream;

/**
 * @given 3 values of different types: uint8_t, uint32_t and uint8_t
 * @when encode is applied
 * @then obtained serialized value meets predefined one
 */
TEST(Scale, EncodeTupleSuccess) {
  uint8_t v1 = 1;
  uint32_t v2 = 2;
  uint8_t v3 = 3;

  ScaleEncoderStream s;
  ASSERT_NO_THROW((s << std::make_tuple(v1, v2, v3)));
  ASSERT_EQ(s.data(), (ByteArray{1, 2, 0, 0, 0, 3}));
}

/**
 * @given byte sequence containign 3 encoded values of
 * different types: uint8_t, uint32_t and uint8_t
 * @when decode is applied
 * @then obtained pair mathces predefined one
 */
TEST(Scale, DecodeTupleSuccess) {
  ByteArray bytes = {1, 2, 0, 0, 0, 3};
  ScaleDecoderStream s(bytes);
  using tuple_type = std::tuple<uint8_t, uint32_t, uint8_t>;
  tuple_type tuple{};
  ASSERT_NO_THROW((s >> tuple));
  auto && [v1, v2, v3] = tuple;
  ASSERT_EQ(v1, 1);
  ASSERT_EQ(v2, 2);
  ASSERT_EQ(v3, 3);
}
