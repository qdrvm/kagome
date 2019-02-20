#include <gtest/gtest.h>
#include "common/buffer.hpp"

using namespace kagome::common;
using namespace std::string_literals;

TEST(Common, Buffer_Put) {
  Buffer b;
  ASSERT_EQ(b.size(), 0);

  auto hex = b.to_hex();
  ASSERT_EQ(hex, ""s);

  b.put("hello");
  ASSERT_EQ(b.size(), 5);

  b.put_uint8(1);
  ASSERT_EQ(b.size(), 6);

  b.put_uint32(0);
  ASSERT_EQ(b.size(), 10);

  b.put_uint64(0);
  ASSERT_EQ(b.size(), 18);

  b.put(std::vector<uint8_t>{1, 2, 3, 4, 5});
  ASSERT_EQ(b.size(), 23);

  // test iterators
  int i = 0;
  for (const auto &byte : b) {
    i++;
  }
  ASSERT_EQ(i, b.size());
}

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
