/**
 * Copyright Quadrivium LLC All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <wasmedge/wasmedge.h>

#include "log/trace_macros.hpp"
#include "runtime/common/memory_allocator.hpp"
#include "runtime/memory.hpp"
#include "runtime/memory_provider.hpp"
#include "runtime/ptr_size.hpp"
#include "runtime/types.hpp"

namespace kagome::runtime::wasm_edge {

  class MemoryImpl final : public MemoryHandle {
   public:
    MemoryImpl(WasmEdge_MemoryInstanceContext *mem_instance);

    /**
     * @brief Return the size of the memory
     */
    WasmSize size() const override {
      return WasmEdge_MemoryInstanceGetPageSize(mem_instance_)
           * kMemoryPageSize;
    }

    std::optional<WasmSize> pagesMax() const override;

    /**
     * Resizes memory to the given size
     * @param new_size
     */
    void resize(WasmSize new_size) override;

    outcome::result<BytesOut> view(WasmPointer ptr,
                                   WasmSize size) const override;

   private:
    WasmEdge_MemoryInstanceContext *mem_instance_;
    log::Logger logger_ = log::createLogger("Memory", "runtime");
  };

  class ExternalMemoryProviderImpl final : public MemoryProvider {
   public:
    explicit ExternalMemoryProviderImpl(
        WasmEdge_MemoryInstanceContext *wasmedge_memory);

    std::optional<std::reference_wrapper<runtime::Memory>> getCurrentMemory()
        const override;

    [[nodiscard]] outcome::result<void> resetMemory(
        const MemoryConfig &config) override;

   private:
    std::optional<std::shared_ptr<Memory>> current_memory_;
    WasmEdge_MemoryInstanceContext *wasmedge_memory_;
  };

  class InternalMemoryProviderImpl final : public MemoryProvider {
   public:
    explicit InternalMemoryProviderImpl() = default;

    std::optional<std::reference_wrapper<runtime::Memory>> getCurrentMemory()
        const override;

    [[nodiscard]] outcome::result<void> resetMemory(
        const MemoryConfig &config) override;

    void setMemory(WasmEdge_MemoryInstanceContext *wasmedge_memory);

   private:
    std::optional<std::shared_ptr<Memory>> current_memory_;
    WasmEdge_MemoryInstanceContext *wasmedge_memory_{};
  };

}  // namespace kagome::runtime::wasm_edge
