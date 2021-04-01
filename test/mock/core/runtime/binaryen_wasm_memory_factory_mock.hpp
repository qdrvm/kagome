/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BINARYEN_WASM_MEMORY_FACTORY_MOCK_HPP
#define KAGOME_BINARYEN_WASM_MEMORY_FACTORY_MOCK_HPP

#include "runtime/binaryen/binaryen_wasm_memory_factory.hpp"

#include <gmock/gmock.h>

namespace kagome::runtime::binaryen {

  class BinaryenWasmMemoryFactoryMock : public BinaryenWasmMemoryFactory {
   public:
    ~BinaryenWasmMemoryFactoryMock() override = default;

    MOCK_CONST_METHOD1(make,
                       std::unique_ptr<WasmMemoryImpl>(
                           wasm::ShellExternalInterface::Memory *));
  };

}  // namespace kagome::runtime::binaryen

#endif  // KAGOME_BINARYEN_WASM_MEMORY_FACTORY_MOCK_HPP
