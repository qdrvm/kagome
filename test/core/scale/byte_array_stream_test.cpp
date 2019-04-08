/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "common/result.hpp"
#include "scale/byte_array_stream.hpp"

using kagome::scale::ByteArray;
using kagome::scale::ByteArrayStream;

/**
 * @given byte array of 3 items: 0, 1, 2
 * @when create BasicStream wrapping this array and start to get bytes one by
 * one
 * @then bytes 0, 1, 2 are obtained sequentially
 */
TEST(ByteArrayStreamTest, NextByteTest) {
  auto bytes = ByteArray{0, 1, 2};
  auto stream = ByteArrayStream{bytes};
  ASSERT_EQ(stream.hasMore(3), true);

  auto byte = stream.nextByte();
  ASSERT_EQ(byte.has_value(), true);
  ASSERT_EQ((*byte), 0);

  ASSERT_EQ(stream.hasMore(2), true);
  byte = stream.nextByte();
  ASSERT_EQ(byte.has_value(), true);
  ASSERT_EQ((*byte), 1);

  ASSERT_EQ(stream.hasMore(1), true);
  byte = stream.nextByte();
  ASSERT_EQ(byte.has_value(), true);
  ASSERT_EQ((*byte), 2);

  ASSERT_EQ(stream.hasMore(1), false);
}

/**
 * @given ByteArrayStream with source ByteArray of size N
 * @when advance N is called on given ByteArrayStream
 * @then advance succeeded
 */
TEST(ByteArrayStreamTest, AdvanceSuccessTest) {
  const size_t n = 42;
  ByteArray bytes(n, '0');
  auto stream = ByteArrayStream{bytes};

  ASSERT_TRUE(stream.advance(bytes.size()));
}

/**
 * @given ByteArrayStream with source ByteArray of size N
 * @when advance N+1 is called on given ByteArrayStream
 * @then advance is failed
 */
TEST(ByteArrayStreamTest, AdvanceFailedTest) {
  const size_t n = 42;
  ByteArray bytes(n, '0');
  auto stream = ByteArrayStream{bytes};

  ASSERT_FALSE(stream.advance(bytes.size() + 1));
}
