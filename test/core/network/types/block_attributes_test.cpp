/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "network/types/block_attributes.hpp"

#include <gtest/gtest.h>
#include "scale/scale.hpp"
#include "testutil/outcome.hpp"
#include "testutil/testparam.hpp"

using kagome::network::BlockAttributes;
using kagome::network::BlockAttributesBits;

using kagome::scale::decode;
using kagome::scale::encode;

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
              static_cast<int>(kagome::scale::DecodeError::UNEXPECTED_VALUE));
  } else {
    EXPECT_OUTCOME_TRUE(val, decode<BlockAttributes>(encoded_value));
    ASSERT_EQ(val, value);
  }
}

using Bits = BlockAttributesBits;
using Attr = BlockAttributes;

using testutil::make_param;

INSTANTIATE_TEST_CASE_P(
    BlockAttributesTestCases,
    BlockAttributesTest,
    ::testing::Values(
        make_param<Attr>({0}, false, {0}),
        make_param<Attr>({1}, false, {Bits::HEADER | 0}),
        make_param<Attr>({3}, false, {Bits::HEADER | Bits::BODY}),
        make_param<Attr>({5}, false, {Bits::HEADER | Bits::RECEIPT}),
        make_param<Attr>({8}, false, {Bits::MESSAGE_QUEUE | 0}),
        make_param<Attr>({16}, false, {Bits::JUSTIFICATION | 0}),
        make_param<Attr>({31},
                         false,
                         {Bits::HEADER | Bits::BODY | Bits::RECEIPT
                          | Bits::MESSAGE_QUEUE | Bits::JUSTIFICATION}),
        make_param<Attr>({64}, true, {64}),
        make_param<Attr>({65}, true, {65}),
        make_param<Attr>({128}, true, {128}),
        make_param<Attr>({255}, true, {255})));
