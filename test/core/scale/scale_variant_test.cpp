/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "common/result.hpp"
#include "scale/basic_stream.hpp"
#include "scale/variant.hpp"

#include <boost/variant/get.hpp>

using namespace kagome::common;         // NOLINT
using namespace kagome::common::scale;  // NOLINT

TEST(Scale, encodeVariant) {
  {
    std::variant<uint8_t, uint32_t> v = static_cast<uint8_t>(1);
    Buffer out;
    auto && res = variant::encodeVariant(v, out);
    ASSERT_TRUE(res);
    Buffer match = {0, 1};
    ASSERT_EQ(out, match);
  }
  {
    std::variant<uint8_t, uint32_t> v = static_cast<uint32_t>(1);
    Buffer out;
    auto && res = variant::encodeVariant(v, out);
    ASSERT_TRUE(res);
    Buffer match = {1, 1, 0, 0, 0};
    ASSERT_EQ(out, match);
  }
}

TEST(Scale, decodeVariant) {
  Buffer match = {0, 1,            // uint8_t{1}
                  1, 1, 0, 0, 0};  // uint32_t{1}

  auto stream = BasicStream{match};
  using Variant = std::variant<uint8_t, uint32_t>;
  auto &&res = variant::decodeVariant<uint8_t, uint32_t>(stream);
  ASSERT_TRUE(res);
  auto &&val = res.value();
  ASSERT_TRUE(std::holds_alternative<uint8_t>(val));
  ASSERT_EQ(*std::get_if<uint8_t>(&val), 1);

  auto &&res1 = variant::decodeVariant<uint8_t, uint32_t>(stream);
  auto && val1 = res1.value();
  ASSERT_TRUE(std::holds_alternative<uint32_t>(val1));
  ASSERT_EQ(*std::get_if<uint32_t>(&val1), 1);
}
