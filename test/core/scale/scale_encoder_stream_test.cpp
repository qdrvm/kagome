/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "scale/scale_decoder_stream.hpp"
#include "scale/types.hpp"

using kagome::scale::ByteArray;
using kagome::scale::ScaleDecoderStream;

/**
 * @given byte array of 3 items: 0, 1, 2
 * @when create BasicStream wrapping this array and start to get bytes one by
 * one
 * @then bytes 0, 1, 2 are obtained sequentially @and next nextByte call returns
 * error
 */
TEST(ByteArrayStreamTest, NextByteSuccessTest) {
  auto bytes = ByteArray{0, 1, 2};
  auto stream = ScaleDecoderStream{bytes};

  for (size_t i = 0; i < bytes.size(); i++) {
    uint8_t byte = 0u;
    ASSERT_NO_THROW((byte = stream.nextByte())) << "Fail in " << i;
    ASSERT_EQ(byte, bytes.at(i)) << "Fail in " << i;
  }
  ASSERT_ANY_THROW(stream.nextByte());
}

/**
 * @given ByteArrayStream with source ByteArray of size N
 * @when advance N is called on given ByteArrayStream
 * @then advance succeeded @and there is no next bytes
 */
TEST(ByteArrayStreamTest, AdvanceSuccessTest) {
  const size_t n = 42;
  ByteArray bytes(n, '0');
  auto stream = ScaleDecoderStream{bytes};

  ASSERT_NO_THROW(stream.advance(bytes.size()));
  ASSERT_FALSE(stream.hasMore(1));
}

/**
 * @given ByteArrayStream with source ByteArray of size N
 * @when advance N+1 is called on given ByteArrayStream
 * @then advance is failed
 */
TEST(ByteArrayStreamTest, AdvanceFailedTest) {
  const size_t n = 42;
  ByteArray bytes(n, '0');
  auto stream = ScaleDecoderStream{bytes};

  ASSERT_ANY_THROW(stream.advance(bytes.size() + 1));
}
