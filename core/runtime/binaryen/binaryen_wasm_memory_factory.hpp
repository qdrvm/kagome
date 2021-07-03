/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BINARYEN_WASM_MEMORY_FACTORY_HPP
#define KAGOME_BINARYEN_WASM_MEMORY_FACTORY_HPP

#include "runtime/binaryen/wasm_memory_impl.hpp"

namespace kagome::runtime::binaryen {

  class BinaryenWasmMemoryFactory {
   public:
    virtual ~BinaryenWasmMemoryFactory() = default;

    virtual std::unique_ptr<WasmMemoryImpl> make(
        wasm::ShellExternalInterface::Memory *memory, WasmSize heap_base) const;
  };

}  // namespace kagome::runtime::binaryen

#endif  // KAGOME_BINARYEN_WASM_MEMORY_FACTORY_HPP
