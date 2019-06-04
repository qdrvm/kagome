/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <gtest/gtest.h>
#include <exception>

#include <boost/exception/all.hpp>
#include <boost/exception/info.hpp>
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
TEST(ScaleDecoderStreamTest, NextByteTest) {
  auto bytes = ByteArray{0, 1, 2};
  auto stream = ScaleDecoderStream{bytes};

  for (size_t i = 0; i < bytes.size(); i++) {
    uint8_t byte = 255u;
    ASSERT_NO_THROW((byte = stream.nextByte())) << "Fail in " << i;
    ASSERT_EQ(byte, bytes.at(i)) << "Fail in " << i;
  }

  ASSERT_ANY_THROW(stream.nextByte());
}

/**
 * @given byte array of two bytes
 * @when taking bytes one by one
 * @then first hasMore() answers that 2 bytes are available, but not 3
 * then only 1 byte is available
 * and then even 1 byte is not available, no bytes left
 */
TEST(ScaleDecoderStreamTest, HasMoreTest) {
  auto bytes = ByteArray{0, 1};
  auto stream = ScaleDecoderStream{bytes};

  ASSERT_TRUE(stream.hasMore(0));
  ASSERT_TRUE(stream.hasMore(1));
  ASSERT_TRUE(stream.hasMore(2));
  ASSERT_FALSE(stream.hasMore(3));

  ASSERT_NO_THROW(stream.nextByte());
  ASSERT_TRUE(stream.hasMore(1));
  ASSERT_FALSE(stream.hasMore(2));

  ASSERT_NO_THROW(stream.nextByte());
  ASSERT_FALSE(stream.hasMore(1));

  ASSERT_ANY_THROW(stream.nextByte());
}
