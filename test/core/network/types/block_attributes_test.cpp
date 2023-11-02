/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/types/block_attributes.hpp"

#include <gtest/gtest.h>
#include "scale/scale.hpp"
#include "testutil/outcome.hpp"
#include "testutil/testparam.hpp"

using kagome::network::BlockAttribute;
using kagome::network::BlockAttributes;

using scale::decode;
using scale::encode;

using BlockAttributesTestParam = testutil::TestParam<BlockAttributes>;

struct BlockAttributesTest
    : public ::testing::TestWithParam<BlockAttributesTestParam> {
  ~BlockAttributesTest() override = default;
};

/**
 * @given list of test params including buffer, condition whether decoding
 * should fail, and decoded value
 * @when decode should fail @and decode function is applied
 * @then result of decoding is a failure
 * @when decode should succeed @and decode function is applied
 * @then result of decoding is success @and decoded value matches expectation
 */
TEST_P(BlockAttributesTest, DecodeBlockAttributes) {
  auto [encoded_value, should_fail, value] = GetParam();
  if (should_fail) {
    EXPECT_OUTCOME_FALSE(err, decode<BlockAttributes>(encoded_value));
    ASSERT_EQ(err.value(),
              static_cast<int>(scale::DecodeError::UNEXPECTED_VALUE));
  } else {
    EXPECT_OUTCOME_TRUE(val, decode<BlockAttributes>(encoded_value));
    ASSERT_EQ(val, value);
  }
}

using Attr = BlockAttribute;

auto P = testutil::make_param<BlockAttributes>;

INSTANTIATE_TEST_SUITE_P(
    BlockAttributesTestCases,
    BlockAttributesTest,
    ::testing::Values(P({0}, false, {}),
                      P({1}, false, {Attr::HEADER}),
                      P({3}, false, {Attr::HEADER | Attr::BODY}),
                      P({5}, false, {Attr::HEADER | Attr::RECEIPT}),
                      P({8}, false, {Attr::MESSAGE_QUEUE}),
                      P({16}, false, {Attr::JUSTIFICATION}),
                      P({31},
                        false,
                        {Attr::HEADER | Attr::BODY | Attr::RECEIPT
                         | Attr::MESSAGE_QUEUE | Attr::JUSTIFICATION}),
                      P({64}, true, BlockAttributes(64)),
                      P({65}, true, BlockAttributes(65)),
                      P({128}, true, BlockAttributes(128)),
                      P({255}, true, BlockAttributes(255))));
