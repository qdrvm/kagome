/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BINARYEN_WASM_MEMORY_FACTORY_MOCK_HPP
#define KAGOME_BINARYEN_WASM_MEMORY_FACTORY_MOCK_HPP

#include "runtime/binaryen/binaryen_memory_factory.hpp"

#include <gmock/gmock.h>

namespace kagome::runtime::binaryen {

  class BinaryenWasmMemoryFactoryMock : public BinaryenMemoryFactory {
   public:
    ~BinaryenWasmMemoryFactoryMock() override = default;

    MOCK_METHOD(std::unique_ptr<MemoryImpl>,
                make,
                (wasm::ShellExternalInterface::Memory * memory,
                 WasmSize heap_base),
                (const, override));
  };

}  // namespace kagome::runtime::binaryen

#endif  // KAGOME_BINARYEN_WASM_MEMORY_FACTORY_MOCK_HPP
