/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "runtime/impl/wasm_memory_stream.hpp"

using kagome::runtime::WasmMemoryImpl;
using kagome::runtime::WasmMemoryStream;

TEST(WasmMemoryStreamTest, NextByteTest) {
  auto bytes = std::vector<uint8_t>(4096, 'c');

  auto memory = std::make_shared<WasmMemoryImpl>(bytes.size());
  for (size_t i = 0; i < bytes.size(); i++) {
    memory->store8(i, bytes.at(0));
  }

  auto stream = WasmMemoryStream{memory};

  for (size_t i = 0; i < bytes.size(); i++) {
    auto byte = stream.nextByte();
    ASSERT_TRUE(byte) << "Fail in " << i;
    ASSERT_EQ(*byte, 'c') << "Fail in " << i;
  }
}
