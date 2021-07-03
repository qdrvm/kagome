/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/binaryen_wasm_memory_factory.hpp"

namespace kagome::runtime::binaryen {

  std::unique_ptr<WasmMemoryImpl> BinaryenWasmMemoryFactory::make(
      wasm::ShellExternalInterface::Memory *memory, WasmSize heap_base) const {
    return std::make_unique<WasmMemoryImpl>(memory, heap_base);
  }

}  // namespace kagome::runtime::binaryen
