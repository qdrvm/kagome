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

#include "common/buffer.hpp"

#include <gtest/gtest.h>
#include <testutil/literals.hpp>

using namespace kagome::common;
using namespace std::string_literals;

/**
 * @given empty buffer
 * @when put different stuff in this buffer
 * @then result matches expectation
 */
TEST(Common, BufferPut) {
  Buffer b;
  ASSERT_EQ(b.size(), 0);

  auto hex = b.toHex();
  ASSERT_EQ(hex, ""s);

  auto s = "hello"s;
  b.put(s);
  ASSERT_EQ(b.size(), 5);

  b.putUint8(1);
  ASSERT_EQ(b.size(), 6);

  b.putUint32(1);
  ASSERT_EQ(b.size(), 10);

  b.putUint64(1);
  ASSERT_EQ(b.size(), 18);

  std::vector<uint8_t> e{1, 2, 3, 4, 5};
  b.put(e);
  ASSERT_EQ(b.size(), 23);

  // test iterators
  int i = 0;
  for (const auto &byte : b) {
    i++;
    (void)byte;
  }
  ASSERT_EQ(i, b.size());

  ASSERT_EQ(b.toHex(), "68656c6c6f010000000100000000000000010102030405");
}

/**
 * @given buffer containing bytes {1,2,3}
 * @when putBuffer is applied with another buffer {4,5,6} as parameter
 * @then content of current buffer changes to {1,2,3,4,5,6}
 */
TEST(Common, putBuffer) {
  Buffer current_buffer = {1, 2, 3};
  Buffer another_buffer = {4, 5, 6};
  auto &buffer = current_buffer.putBuffer(another_buffer);
  ASSERT_EQ(&buffer, &current_buffer);  // line to the same buffer is returned
  Buffer result = {1, 2, 3, 4, 5, 6};
  ASSERT_EQ(buffer, result);
}

/**
 * @when create buffer using different constructors
 * @then expected buffer is created
 */
TEST(Common, BufferInit) {
  Buffer b{1, 2, 3, 4, 5};
  ASSERT_EQ(b.size(), 5);
  ASSERT_EQ(b.toHex(), "0102030405"s);

  Buffer a(b);
  ASSERT_EQ(a, b);
  ASSERT_EQ(a.size(), b.size());

  ASSERT_NO_THROW({
    Buffer c{"0102030405"_unhex};
    ASSERT_EQ(c, a);

    Buffer d = c;
    ASSERT_EQ(d, c);
  });
}
