/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_WASMEDGE_MEMORY_PROVIDER_HPP
#define KAGOME_CORE_RUNTIME_WASMEDGE_MEMORY_PROVIDER_HPP

#include "runtime/memory_provider.hpp"

#include "runtime/wasmedge/memory_factory.hpp"

struct WasmEdge_ImportObjectContext;

namespace kagome::runtime::wasmedge {

  class WasmedgeMemoryProvider final : public MemoryProvider {
   public:
    WasmedgeMemoryProvider(
        std::shared_ptr<const WasmedgeMemoryFactory> memory_factory);

    boost::optional<runtime::Memory &> getCurrentMemory() const override;

    [[nodiscard]] outcome::result<void> resetMemory(
        WasmSize heap_base) override;

    void setExternalInterface(WasmEdge_ImportObjectContext *impObj);

   private:
    std::shared_ptr<const WasmedgeMemoryFactory> memory_factory_;
    std::shared_ptr<MemoryImpl> memory_;
    WasmEdge_ImportObjectContext *imp_obj_;
  };

}  // namespace kagome::runtime::wasmedge

#endif  // KAGOME_CORE_RUNTIME_WASMEDGE_MEMORY_PROVIDER_HPP
