/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/buffer.hpp"
#include <gtest/gtest.h>

using namespace kagome::common;
using namespace std::string_literals;

/**
 * @given empty buffer
 * @when put different stuff in this buffer
 * @then result matches expectation
 */
TEST(Common, Buffer_Put) {
  Buffer b;
  ASSERT_EQ(b.size(), 0);

  auto hex = b.to_hex();
  ASSERT_EQ(hex, ""s);

  auto s = "hello"s;
  b.put_bytes(s.begin(), s.end());
  ASSERT_EQ(b.size(), 5);

  b.put_uint8(1);
  ASSERT_EQ(b.size(), 6);

  b.put_uint32(1);
  ASSERT_EQ(b.size(), 10);

  b.put_uint64(1);
  ASSERT_EQ(b.size(), 18);

  std::vector<uint8_t> e{1, 2, 3, 4, 5};
  b.put_bytes(e.begin(), e.end());
  ASSERT_EQ(b.size(), 23);

  // test iterators
  int i = 0;
  for (const auto &byte : b) {
    i++;
  }
  ASSERT_EQ(i, b.size());

  ASSERT_EQ(b.to_hex(), "68656C6C6F010000000100000000000000010102030405");
}

/**
 * @when create buffer using different constructors
 * @then expected buffer is created
 */
TEST(Common, Buffer_Init) {
  Buffer b{1, 2, 3, 4, 5};
  ASSERT_EQ(b.size(), 5);
  ASSERT_EQ(b.to_hex(), "0102030405"s);

  Buffer a(b);
  ASSERT_EQ(a, b);
  ASSERT_EQ(a.size(), b.size());

  Buffer c = Buffer::from_hex("0102030405");
  ASSERT_EQ(c, a);

  Buffer d = c;
  ASSERT_EQ(d, c);
}
