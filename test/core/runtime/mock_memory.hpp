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

#ifndef KAGOME_TEST_CORE_RUNTIME_MOCK_MEMORY_HPP_
#define KAGOME_TEST_CORE_RUNTIME_MOCK_MEMORY_HPP_

#include <gmock/gmock.h>
#include <boost/optional.hpp>
#include "runtime/wasm_memory.hpp"

namespace kagome::runtime {

  class MockMemory : public WasmMemory {
   public:
    MOCK_CONST_METHOD0(size, SizeType());
    MOCK_METHOD1(resize, void(SizeType));
    MOCK_METHOD1(allocate, WasmPointer(SizeType));
    MOCK_METHOD1(deallocate, boost::optional<SizeType>(WasmPointer));

    MOCK_CONST_METHOD1(load8s, int8_t(WasmPointer));
    MOCK_CONST_METHOD1(load8u, uint8_t(WasmPointer));
    MOCK_CONST_METHOD1(load16s, int16_t(WasmPointer));
    MOCK_CONST_METHOD1(load16u, uint16_t(WasmPointer));
    MOCK_CONST_METHOD1(load32s, int32_t(WasmPointer));
    MOCK_CONST_METHOD1(load32u, uint32_t(WasmPointer));
    MOCK_CONST_METHOD1(load64s, int64_t(WasmPointer));
    MOCK_CONST_METHOD1(load64u, uint64_t(WasmPointer));
    MOCK_CONST_METHOD1(load128, std::array<uint8_t, 16>(WasmPointer));
    MOCK_CONST_METHOD2(loadN, common::Buffer(WasmPointer, SizeType));

    MOCK_METHOD2(store8, void(WasmPointer, int8_t));
    MOCK_METHOD2(store16, void(WasmPointer, int16_t));
    MOCK_METHOD2(store32, void(WasmPointer, int32_t));
    MOCK_METHOD2(store64, void(WasmPointer, int64_t));
    MOCK_METHOD2(store128, void(WasmPointer, const std::array<uint8_t, 16> &));
    MOCK_METHOD2(storeBuffer, void(WasmPointer, const common::Buffer &));
  };

}  // namespace kagome::runtime

#endif  // KAGOME_TEST_CORE_RUNTIME_MOCK_MEMORY_HPP_
