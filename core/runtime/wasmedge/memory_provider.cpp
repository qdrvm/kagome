/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wasmedge/memory_provider.hpp"

#include "runtime/common/memory_allocator.hpp"
#include "runtime/wasmedge/memory_impl.hpp"

#include <wasmedge.h>

namespace kagome::runtime::wasmedge {

  boost::optional<runtime::Memory &> WasmedgeMemoryProvider::getCurrentMemory()
      const {
    return memory_ == nullptr ? boost::none
                              : boost::optional<Memory &>{*memory_};
  }

  outcome::result<void> WasmedgeMemoryProvider::resetMemory(WasmSize) {
    memory_ = std::make_unique<MemoryImpl>(mem_ctx_);
    return outcome::success();
  }

  void WasmedgeMemoryProvider::setExternalInterface(
      WasmEdge_ImportObjectContext *imp_obj) {
    WasmEdge_Limit MemoryLimit = {.HasMax = false, .Min = 40, .Max = 40};
    mem_ctx_ = WasmEdge_MemoryInstanceCreate(MemoryLimit);
    WasmEdge_String MemoryName = WasmEdge_StringCreateByCString("memory");
    WasmEdge_ImportObjectAddMemory(imp_obj, MemoryName, mem_ctx_);
    WasmEdge_StringDelete(MemoryName);
    resetMemory(0);
  }

}  // namespace kagome::runtime::wasmedge
