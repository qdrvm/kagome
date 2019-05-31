/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "scale/scale.hpp"

#include <gsl/span>
#include "testutil/outcome.hpp"

using kagome::scale::ByteArray;
using kagome::scale::decode;
using kagome::scale::encode;

// TODO(yuraz): PRE-*** refactor (parameterize) tests, add negative tests
// TODO(yuraz): PRE-*** add descriptions

TEST(Scale, encodeVariant) {
  {
    boost::variant<uint8_t, uint32_t> v = static_cast<uint8_t>(1);
    EXPECT_OUTCOME_TRUE(data, encode(v))
    ASSERT_EQ(data, (ByteArray{0, 1}));
  }
  {
    boost::variant<uint8_t, uint32_t> v = static_cast<uint32_t>(1);
    EXPECT_OUTCOME_TRUE(data, encode(v))
    ASSERT_EQ(data, (ByteArray{1, 1, 0, 0, 0}));
  }
}

TEST(Scale, decodeVariant) {
  {
    ByteArray match = {0, 1};  // uint8_t{1}
    using variant_type = boost::variant<uint8_t, uint32_t>;
    EXPECT_OUTCOME_TRUE(val, decode<variant_type>(match))
    kagome::visit_in_place(
        val, [](uint8_t v) { ASSERT_EQ(v, 1); }, [](uint32_t v) { FAIL(); });
  }
  {
    ByteArray match = {1, 1, 0, 0, 0};  // uint32_t{1}
    using variant_type = boost::variant<uint8_t, uint32_t>;
    EXPECT_OUTCOME_TRUE(val, decode<variant_type>(match))
    kagome::visit_in_place(
        val, [](uint32_t v) { ASSERT_EQ(v, 1); }, [](uint8_t v) { FAIL(); });
  }
}
