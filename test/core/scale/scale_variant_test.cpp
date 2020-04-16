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

#include <gsl/span>
#include "testutil/outcome.hpp"

using kagome::scale::ByteArray;
using kagome::scale::decode;
using kagome::scale::encode;
using kagome::scale::ScaleDecoderStream;
using kagome::scale::ScaleEncoderStream;

class VariantFixture
    : public testing::TestWithParam<
          std::pair<boost::variant<uint8_t, uint32_t>, ByteArray>> {
 protected:
  ScaleEncoderStream s;
};
namespace {
  std::pair<boost::variant<uint8_t, uint32_t>, ByteArray> make_pair(
      boost::variant<uint8_t, uint32_t> v, ByteArray m) {
    return std::pair<boost::variant<uint8_t, uint32_t>, ByteArray>(
        std::move(v), std::move(m));
  }
}  // namespace

/**
 * @given variant value and byte array
 * @when value is scale-encoded
 * @then encoded bytes match predefined byte array
 */
TEST_P(VariantFixture, EncodeSuccessTest) {
  const auto &[value, match] = GetParam();
  ASSERT_NO_THROW(s << value);
  ASSERT_EQ(s.data(), match);
}

INSTANTIATE_TEST_CASE_P(CompactTestCases, VariantFixture,
                        ::testing::Values(make_pair(uint8_t(1), {0, 1}),
                                          make_pair(uint32_t(2),
                                                    {1, 2, 0, 0, 0})));

/**
 * @given byte array of encoded variant of types uint8_t and uint32_t
 * containing uint8_t value
 * @when variant decoded from scale decoder stream
 * @then obtained varian has alternative type uint8_t and is equal to encoded
 * uint8_t value
 */
TEST(ScaleVariant, DecodeU8Success) {
  ByteArray match = {0, 1};  // uint8_t{1}
  ScaleDecoderStream s(match);
  boost::variant<uint8_t, uint32_t> val{};
  ASSERT_NO_THROW(s >> val);
  kagome::visit_in_place(
      val, [](uint8_t v) { ASSERT_EQ(v, 1); }, [](uint32_t v) { FAIL(); });
}

/**
 * @given byte array of encoded variant of types uint8_t and uint32_t
 * containing uint32_t value
 * @when variant decoded from scale decoder stream
 * @then obtained varian has alternative type uint32_t and is equal to encoded
 * uint32_t value
 */
TEST(ScaleVariant, DecodeU32Success) {
  ByteArray match = {1, 1, 0, 0, 0};  // uint32_t{1}
  ScaleDecoderStream s(match);
  boost::variant<uint8_t, uint32_t> val{};
  ASSERT_NO_THROW(s >> val);
  kagome::visit_in_place(
      val, [](uint32_t v) { ASSERT_EQ(v, 1); }, [](uint8_t v) { FAIL(); });
}
