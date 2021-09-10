/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_WASMEDGE_MEMORY_PROVIDER_HPP
#define KAGOME_CORE_RUNTIME_WASMEDGE_MEMORY_PROVIDER_HPP

#include "runtime/memory_provider.hpp"

struct WasmEdge_ImportObjectContext;
struct WasmEdge_MemoryInstanceContext;

namespace kagome::runtime::wasmedge {

  class MemoryImpl;

  class WasmedgeMemoryProvider final : public MemoryProvider {
   public:
    boost::optional<runtime::Memory &> getCurrentMemory() const override;

    [[nodiscard]] outcome::result<void> resetMemory(
        WasmSize heap_base) override;

    void setExternalInterface(WasmEdge_ImportObjectContext *impObj);

   private:
    std::shared_ptr<MemoryImpl> memory_;
    WasmEdge_MemoryInstanceContext* mem_ctx_;
  };

}  // namespace kagome::runtime::wasmedge

#endif  // KAGOME_CORE_RUNTIME_WASMEDGE_MEMORY_PROVIDER_HPP
