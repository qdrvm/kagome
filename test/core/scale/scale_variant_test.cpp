/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include "scale/byte_array_stream.hpp"
#include "scale/variant.hpp"
#include "scale/scale_encoder_stream.hpp"

using kagome::common::Buffer;
using kagome::scale::ByteArray;
using kagome::scale::ByteArrayStream;
using kagome::scale::variant::decodeVariant;
using kagome::scale::variant::encodeVariant;
using kagome::scale::ScaleEncoderStream;

TEST(Scale, encodeVariant) {
  {
    boost::variant<uint8_t, uint32_t> v = static_cast<uint8_t>(1);
    ScaleEncoderStream s;
    ASSERT_NO_THROW(encodeVariant(v, s));
    ASSERT_EQ(s.data(), (ByteArray{0, 1}));
  }
  {
    boost::variant<uint8_t, uint32_t> v = static_cast<uint32_t>(1);
    ScaleEncoderStream s;
    ASSERT_NO_THROW(encodeVariant(v, s));
    ASSERT_EQ(s.data(), (ByteArray{1, 1, 0, 0, 0}));
  }
}

TEST(Scale, decodeVariant) {
  Buffer match = {0, 1,            // uint8_t{1}
                  1, 1, 0, 0, 0};  // uint32_t{1}

  auto stream = ByteArrayStream{match};
  auto &&res = decodeVariant<uint8_t, uint32_t>(stream);
  ASSERT_TRUE(res);
  auto &&val = res.value();

  kagome::visit_in_place(val, [](uint8_t v) { ASSERT_EQ(v, 1); },
                         [](uint32_t v) { FAIL(); });

  auto &&res1 = decodeVariant<uint8_t, uint32_t>(stream);
  auto &&val1 = res1.value();

  kagome::visit_in_place(val1, [](uint32_t v) { ASSERT_EQ(v, 1); },
                         [](uint8_t v) { FAIL(); });
}
