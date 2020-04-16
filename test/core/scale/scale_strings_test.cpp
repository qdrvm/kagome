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

#include <gtest/gtest.h>

#include "scale/scale.hpp"
#include "testutil/outcome.hpp"

using kagome::scale::ByteArray;
using kagome::scale::ScaleDecoderStream;
using kagome::scale::ScaleEncoderStream;

/**
 * @given raw string
 * @when specified string is encoded by ScaleEncoderStream
 * @then encoded value meets expectations
 */
TEST(Scale, RawStringEncodeSuccess) {
auto * v = "asdadad";
ScaleEncoderStream s{};
ASSERT_NO_THROW((s << v));
ASSERT_EQ(s.data(), (ByteArray{28, 'a', 's', 'd', 'a', 'd', 'a', 'd'}));
}

/**
 * @given raw string
 * @when specified string is encoded by ScaleEncoderStream
 * @then encoded value meets expectations
 */
TEST(Scale, StdStringEncodeSuccess) {
std::string v = "asdadad";
ScaleEncoderStream s;
ASSERT_NO_THROW((s << v));
ASSERT_EQ(s.data(), (ByteArray{28, 'a', 's', 'd', 'a', 'd', 'a', 'd'}));
}

/**
 * @given byte array containing encoded string
 * @when string is decoded using ScaleDecoderStream
 * @then decoded string matches expectations
 */
TEST(Scale, StringDecodeSuccess) {
auto bytes = ByteArray{28, 'a', 's', 'd', 'a', 'd', 'a', 'd'};
ScaleDecoderStream s(bytes);
std::string v;
ASSERT_NO_THROW(s >> v);
ASSERT_EQ(v, "asdadad");
}
