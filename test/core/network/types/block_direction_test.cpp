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

#include "network/types/block_direction.hpp"

#include <gtest/gtest.h>
#include "scale/scale.hpp"
#include "testutil/outcome.hpp"
#include "testutil/testparam.hpp"

using kagome::network::Direction;

using kagome::scale::decode;
using kagome::scale::encode;

using DirectionTestParam = testutil::TestParam<Direction>;

struct DirectionTest : public ::testing::TestWithParam<DirectionTestParam> {};

/**
 * @given list of test params including buffer, condition whether decoding
 * should fail, and decoded value
 * @when decode should fail @and decode function is applied
 * @then result of decoding is a failure
 * @when decode should succeed @and decode function is applied
 * @then result of decoding is success @and decoded value matches expectation
 */
TEST_P(DirectionTest, DecodeDirection) {
  auto [encoded_value, should_fail, value] = GetParam();
  if (should_fail) {
    EXPECT_OUTCOME_FALSE(err, decode<Direction>(encoded_value));
    ASSERT_EQ(err.value(),
              static_cast<int>(kagome::scale::DecodeError::UNEXPECTED_VALUE));
  } else {
    EXPECT_OUTCOME_TRUE(val, decode<Direction>(encoded_value));
    ASSERT_EQ(val, value);
  }
}

using testutil::make_param;

INSTANTIATE_TEST_CASE_P(
    DirectionTestCases,
    DirectionTest,
    ::testing::Values(
        make_param<Direction>({0}, false, Direction::ASCENDING),
        make_param<Direction>({1}, false, Direction::DESCENDING),
        make_param<Direction>({2}, true, static_cast<Direction>(2)),
        make_param<Direction>({3}, true, static_cast<Direction>(3)),
        make_param<Direction>({111}, true, static_cast<Direction>(111)),
        make_param<Direction>({255}, true, static_cast<Direction>(255))));
