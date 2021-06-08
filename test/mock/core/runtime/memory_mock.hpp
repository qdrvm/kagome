/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_CORE_RUNTIME_WASM_MEMORY_MOCK
#define KAGOME_TEST_MOCK_CORE_RUNTIME_WASM_MEMORY_MOCK

#include "runtime/memory.hpp"

#include <gmock/gmock.h>

namespace kagome::runtime {

  class MemoryMock : public Memory {
   public:
    MOCK_METHOD1(setHeapBase, void(WasmSize));

    MOCK_METHOD0(reset, void());
    MOCK_CONST_METHOD0(size, WasmSize());
    MOCK_METHOD1(resize, void(WasmSize));
    MOCK_METHOD1(allocate, WasmPointer(WasmSize));
    MOCK_METHOD1(deallocate, boost::optional<WasmSize>(WasmPointer));

    MOCK_CONST_METHOD1(load8s, int8_t(WasmPointer));
    MOCK_CONST_METHOD1(load8u, uint8_t(WasmPointer));
    MOCK_CONST_METHOD1(load16s, int16_t(WasmPointer));
    MOCK_CONST_METHOD1(load16u, uint16_t(WasmPointer));
    MOCK_CONST_METHOD1(load32s, int32_t(WasmPointer));
    MOCK_CONST_METHOD1(load32u, uint32_t(WasmPointer));
    MOCK_CONST_METHOD1(load64s, int64_t(WasmPointer));
    MOCK_CONST_METHOD1(load64u, uint64_t(WasmPointer));
    MOCK_CONST_METHOD1(load128, std::array<uint8_t, 16>(WasmPointer));
    MOCK_CONST_METHOD2(loadN, common::Buffer(WasmPointer, WasmSize));
    MOCK_CONST_METHOD2(loadStr, std::string(WasmPointer, WasmSize));

    MOCK_METHOD2(store8, void(WasmPointer, int8_t));
    MOCK_METHOD2(store16, void(WasmPointer, int16_t));
    MOCK_METHOD2(store32, void(WasmPointer, int32_t));
    MOCK_METHOD2(store64, void(WasmPointer, int64_t));
    MOCK_METHOD2(store128, void(WasmPointer, const std::array<uint8_t, 16> &));
    MOCK_METHOD2(storeBuffer, void(WasmPointer, gsl::span<const uint8_t>));
    MOCK_METHOD1(storeBuffer, WasmSpan(gsl::span<const uint8_t>));
  };

}  // namespace kagome::runtime

#endif  // KAGOME_TEST_MOCK_CORE_RUNTIME_WASM_MEMORY_MOCK
