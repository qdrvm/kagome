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

#include "scale/scale.hpp"

#include <gtest/gtest.h>

using kagome::scale::ByteArray;
using kagome::scale::ScaleDecoderStream;
using kagome::scale::ScaleEncoderStream;

/**
 * @given pair of values of different types: uint8_t and uint32_t
 * @when encode is applied
 * @then obtained serialized value meets predefined one
 */
TEST(Scale, encodePair) {
  uint8_t v1 = 1;
  uint32_t v2 = 2;

  ScaleEncoderStream s;
  ASSERT_NO_THROW((s << std::make_pair(v1, v2)));
  ASSERT_EQ(s.data(), (ByteArray{1, 2, 0, 0, 0}));
}

/**
 * @given byte sequence containign 2 encoded values of
 * different types: uint8_t and uint32_t
 * @when decode is applied
 * @then obtained pair mathces predefined one
 */
TEST(Scale, decodePair) {
  ByteArray bytes = {1, 2, 0, 0, 0};
  ScaleDecoderStream s(bytes);
  using pair_type = std::pair<uint8_t, uint32_t>;
  pair_type pair{};
  ASSERT_NO_THROW((s >> pair));
  ASSERT_EQ(pair.first, 1);
  ASSERT_EQ(pair.second, 2);
}
