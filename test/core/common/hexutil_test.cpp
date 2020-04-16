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

#include "common/hexutil.hpp"

#include <gtest/gtest.h>
#include "testutil/literals.hpp"

using namespace kagome::common;
using namespace std::string_literals;

/**
 * @given Array of bytes
 * @when hex it
 * @then hex matches expected encoding
 */
TEST(Common, Hexutil_Hex) {
  auto bin = "00010204081020FF"_unhex;
  auto hexed = hex_upper(bin);
  ASSERT_EQ(hexed, "00010204081020FF"s);
}

/**
 * @given Hexencoded string of even length
 * @when unhex
 * @then no exception, result matches expected value
 */
TEST(Common, Hexutil_UnhexEven) {
  auto s = "00010204081020ff"s;

  std::vector<uint8_t> actual;
  ASSERT_NO_THROW(
      actual = unhex(s).value())
      << "unhex result does not contain expected std::vector<uint8_t>";

  auto expected = "00010204081020ff"_unhex;

  ASSERT_EQ(actual, expected);
}

/**
 * @given Hexencoded string of odd length
 * @when unhex
 * @then unhex result contains error
 */
TEST(Common, Hexutil_UnhexOdd) {
  ASSERT_NO_THROW({
    unhex("0").error();
  }) << "unhex did not return an error as expected";
}

/**
 * @given Hexencoded string with non-hex letter
 * @when unhex
 * @then unhex result contains error
 */
TEST(Common, Hexutil_UnhexInvalid) {
  ASSERT_NO_THROW({
    unhex("keks").error();
  }) << "unhex did not return an error as expected";
}
