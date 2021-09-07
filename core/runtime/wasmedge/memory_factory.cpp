/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wasmedge/memory_factory.hpp"

#include "runtime/common/memory_allocator.hpp"

namespace kagome::runtime::wasmedge {

  std::unique_ptr<MemoryImpl> WasmedgeMemoryFactory::make(
      WasmEdge_MemoryInstanceContext *memory) const {
    return std::make_unique<MemoryImpl>(memory);
  }

}  // namespace kagome::runtime::wasmedge
