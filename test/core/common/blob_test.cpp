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

#include "common/blob.hpp"
#include <gtest/gtest.h>

using namespace kagome::common;

/**
 * @given hex string
 * @when create blob object from this string using fromHex method
 * @then blob object is created and contains expected byte representation of the
 * hex string
 */
TEST(BlobTest, CreateFromValidHex) {
  std::string hex32 = "00ff";
  std::array<byte_t, 2> expected{0, 255};

  auto result = Blob<2>::fromHex(hex32);
  ASSERT_NO_THROW({
    auto blob = result.value();
    EXPECT_EQ(blob, expected);
  }) << "fromHex returned an error instead of value";
}

/**
 * @given non hex string
 * @when try to create a Blob using fromHex on that string
 * @then error is returned
 */
TEST(BlobTest, CreateFromNonHex) {
  std::string not_hex = "nothex";

  auto result = Blob<2>::fromHex(not_hex);
  ASSERT_NO_THROW({ result.error(); })
      << "fromHex returned a value instead of error";
}

/**
 * @given string with odd length
 * @when try to create a Blob using fromHex on that string
 * @then error is returned
 */
TEST(BlobTest, CreateFromOddLengthHex) {
  std::string odd_hex = "0a1";

  auto result = Blob<2>::fromHex(odd_hex);
  ASSERT_NO_THROW({ result.error(); })
      << "fromHex returned a value instead of error";
}

/**
 * @given string with odd length
 * @when try to create a Blob using fromHex on that string
 * @then error is returned
 */
TEST(BlobTest, CreateFromWrongLendthHex) {
  std::string odd_hex = "00ff00";

  auto result = Blob<2>::fromHex(odd_hex);
  ASSERT_NO_THROW({ result.error(); })
      << "fromHex returned a value instead of error";
}

/**
 * @given arbitrary string
 * @when create blob object from this string using fromString method
 * @then blob object is created and contains expected byte representation of
 * given string
 */
TEST(BlobTest, CreateFromValidString) {
  std::array<byte_t, 5> expected{'a', 's', 'd', 'f', 'g'};
  std::string valid_str{expected.begin(), expected.end()};

  auto result = Blob<5>::fromString(valid_str);
  ASSERT_NO_THROW({
    auto blob = result.value();
    EXPECT_EQ(blob, expected);
  }) << "fromString returned an error instead of value";
}

/**
 * @given arbitrary string
 * @when create blob object from this string using fromString method where @and
 * length of the string is not equal to the size of the blob
 * @then blob object is not created, fromString method returns error message
 * error
 */
TEST(BlobTest, CreateFromInvalidString) {
  std::string valid_str{"0"};

  auto result = Blob<5>::fromString(valid_str);
  ASSERT_NO_THROW({ result.error(); })
      << "fromString returned a value instead of error";
}

/**
 * @given arbitrary string and its hex representation
 * @when blob is created from that string
 * @then blob::toHex() method returns given hex representation
 */
TEST(BlobTest, ToHexTest) {
  std::string str = "hello";
  std::string expected_hex = "68656c6c6f";

  auto blob_res = Blob<5>::fromString(str);
  ASSERT_NO_THROW({
    Blob<5> value = blob_res.value();
    ASSERT_EQ(value.toHex(), expected_hex);
  });
}

/**
 * @given byte array with characters
 * @when blob is created from that byte array
 * @then blob.toString() returns the string made of that characters
 */
TEST(BlobTest, ToStringTest) {
  std::array<byte_t, 5> expected{'a', 's', 'd', 'f', 'g'};

  Blob<5> blob;
  std::copy(expected.begin(), expected.end(), blob.begin());

  ASSERT_EQ(blob.toString(), std::string(expected.begin(), expected.end()));
}
