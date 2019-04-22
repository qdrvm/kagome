/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "core/runtime/mock_memory.hpp"
#include "runtime/impl/wasm_memory_stream.hpp"

using kagome::runtime::MockMemory;
using kagome::runtime::WasmMemoryStream;

using testing::_;
using testing::Return;

/**
 * @given byte array of size 4096
 * @when create BasicStream wrapping this array and start to get bytes one by
 * one
 * @then bytes 0, 1, 2 are obtained sequentially
 */
TEST(WasmMemoryStreamTest, NextByteTest) {
  const size_t kMemorySize = 4096;

  auto memory = std::make_shared<MockMemory>();

  EXPECT_CALL(*memory, load8u(_)).WillRepeatedly(Return('c'));
  EXPECT_CALL(*memory, size()).WillRepeatedly(Return(kMemorySize));

  auto stream = WasmMemoryStream{memory};

  for (size_t i = 0; i < kMemorySize; i++) {
    auto byte = stream.nextByte();
    ASSERT_TRUE(byte) << "Fail in " << i;
    ASSERT_EQ(*byte, 'c') << "Fail in " << i;
  }
  ASSERT_FALSE(stream.nextByte());
}

/**
 * @given ByteArrayStream with source ByteArray of size N
 * @when advance N is called on given ByteArrayStream
 * @then advance succeeded
 */
TEST(WasmMemoryStreamTest, AdvanceSuccessTest) {
  const size_t kMemorySize = 4096;

  auto memory = std::make_shared<MockMemory>();
  EXPECT_CALL(*memory, size()).WillRepeatedly(Return(kMemorySize));
  auto stream = WasmMemoryStream{memory};

  ASSERT_TRUE(stream.advance(kMemorySize));
}

/**
 * @given ByteArrayStream with source ByteArray of size N
 * @when advance N+1 is called on given ByteArrayStream
 * @then advance is failed
 */
TEST(WasmMemoryStreamTest, AdvanceFailedTest) {
  const size_t kMemorySize = 4096;

  auto memory = std::make_shared<MockMemory>();
  EXPECT_CALL(*memory, size()).WillRepeatedly(Return(kMemorySize));
  auto stream = WasmMemoryStream{memory};

  ASSERT_FALSE(stream.advance(kMemorySize + 1));
}
