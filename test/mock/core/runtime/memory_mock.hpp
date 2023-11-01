/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/memory.hpp"

#include <gmock/gmock.h>

namespace kagome::runtime {

  class MemoryMock : public Memory {
   public:
    MOCK_METHOD(void, setHeapBase, (WasmSize), ());

    MOCK_METHOD(void, reset, ());

    MOCK_METHOD(WasmSize, size, (), (const, override));

    MOCK_METHOD(void, resize, (WasmSize), (override));

    MOCK_METHOD(WasmPointer, allocate, (WasmSize), (override));

    MOCK_METHOD(std::optional<WasmSize>, deallocate, (WasmPointer), (override));

    MOCK_METHOD(int8_t, load8s, (WasmPointer), (const, override));

    MOCK_METHOD(uint8_t, load8u, (WasmPointer), (const, override));

    MOCK_METHOD(int16_t, load16s, (WasmPointer), (const, override));

    MOCK_METHOD(uint16_t, load16u, (WasmPointer), (const, override));

    MOCK_METHOD(int32_t, load32s, (WasmPointer), (const, override));

    MOCK_METHOD(uint32_t, load32u, (WasmPointer), (const, override));

    MOCK_METHOD(int64_t, load64s, (WasmPointer), (const, override));

    MOCK_METHOD(uint64_t, load64u, (WasmPointer), (const, override));

    MOCK_METHOD((std::array<uint8_t, 16>),
                load128,
                (WasmPointer),
                (const, override));

    MOCK_METHOD(common::BufferView,
                loadN,
                (WasmPointer, WasmSize),
                (const, override));

    MOCK_METHOD(std::string,
                loadStr,
                (WasmPointer, WasmSize),
                (const, override));

    MOCK_METHOD(void, store8, (WasmPointer, int8_t), (override));

    MOCK_METHOD(void, store16, (WasmPointer, int16_t), (override));

    MOCK_METHOD(void, store32, (WasmPointer, int32_t), (override));

    MOCK_METHOD(void, store64, (WasmPointer, int64_t), (override));

    MOCK_METHOD(void,
                store128,
                (WasmPointer, (const std::array<uint8_t, 16> &)),
                (override));

    MOCK_METHOD(void,
                storeBuffer,
                (WasmPointer, common::BufferView),
                (override));

    MOCK_METHOD(WasmSpan, storeBuffer, (common::BufferView), (override));
  };

}  // namespace kagome::runtime
