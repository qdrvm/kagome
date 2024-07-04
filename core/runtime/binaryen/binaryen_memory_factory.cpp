/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/binaryen_memory_factory.hpp"

#include "runtime/common/memory_allocator.hpp"

namespace kagome::runtime::binaryen {

  std::unique_ptr<MemoryImpl> BinaryenMemoryFactory::make(
      RuntimeExternalInterface::InternalMemory *memory) const {
    return std::make_unique<MemoryImpl>(memory);
  }

}  // namespace kagome::runtime::binaryen
