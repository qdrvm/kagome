/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/binaryen_memory_factory.hpp"

#include "runtime/common/memory_allocator.hpp"

namespace kagome::runtime::binaryen {

  std::unique_ptr<MemoryImpl> BinaryenMemoryFactory::make(
      wasm::ShellExternalInterface::Memory *memory, WasmSize heap_base) const {
    return std::make_unique<MemoryImpl>(memory, heap_base);
  }

}  // namespace kagome::runtime::binaryen
