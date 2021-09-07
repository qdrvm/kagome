/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wasmedge/memory_provider.hpp"

#include "runtime/common/memory_allocator.hpp"

#include <wasmedge.h>

namespace kagome::runtime::wasmedge {

  WasmedgeMemoryProvider::WasmedgeMemoryProvider(
      std::shared_ptr<const WasmedgeMemoryFactory> memory_factory)
      : memory_factory_{std::move(memory_factory)} {
    BOOST_ASSERT(memory_factory_);
  }

  boost::optional<runtime::Memory &> WasmedgeMemoryProvider::getCurrentMemory()
      const {
    return memory_ == nullptr ? boost::none
                              : boost::optional<Memory &>{*memory_};
  }

  outcome::result<void> WasmedgeMemoryProvider::resetMemory(
      WasmSize heap_base) {
    if (store_ctx_) {
      WasmEdge_String MemoryName = WasmEdge_StringCreateByCString("memory");
      auto memory = WasmEdge_StoreFindMemory(store_ctx_, MemoryName);
      if(memory) {
        memory_ = memory_factory_->make(memory);
      }
    }
    return outcome::success();
  }

  void WasmedgeMemoryProvider::setExternalInterface(
      WasmEdge_StoreContext *storeCtx) {
    store_ctx_ = storeCtx;
  }

}  // namespace kagome::runtime::wasmedge
