/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wasmedge/memory_factory.hpp"

#include "runtime/common/memory_allocator.hpp"

#include <wasmedge.h>

namespace kagome::runtime::wasmedge {

  std::unique_ptr<MemoryImpl> WasmedgeMemoryFactory::make(
      WasmEdge_ImportObjectContext *obj) const {
    WasmEdge_Limit MemoryLimit = {.HasMax = false, .Min = 40, .Max = 40};
    WasmEdge_MemoryInstanceContext *HostMemory =
        WasmEdge_MemoryInstanceCreate(MemoryLimit);
    WasmEdge_String MemoryName = WasmEdge_StringCreateByCString("memory");
    WasmEdge_ImportObjectAddMemory(obj, MemoryName, HostMemory);
    WasmEdge_StringDelete(MemoryName);
    return std::make_unique<MemoryImpl>(HostMemory);
  }

}  // namespace kagome::runtime::wasmedge
