/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_BINARYEN_BINARYEN_MEMORY_PROVIDER_HPP
#define KAGOME_CORE_RUNTIME_BINARYEN_BINARYEN_MEMORY_PROVIDER_HPP

#include "runtime/binaryen/binaryen_memory_provider.hpp"
#include "runtime/binaryen/binaryen_wasm_memory_factory.hpp"
#include "runtime/binaryen/runtime_external_interface.hpp"
#include "runtime/memory_provider.hpp"

namespace wasm {
  class RuntimeExternalInterface;
}

namespace kagome::runtime::binaryen {

  class BinaryenMemoryProvider final : public MemoryProvider {
   public:
    BinaryenMemoryProvider(
        std::shared_ptr<const BinaryenWasmMemoryFactory> memory_factory);

    boost::optional<runtime::Memory&> getCurrentMemory() const override;
    void resetMemory(WasmSize heap_base) override;

    void setExternalInterface(std::shared_ptr<RuntimeExternalInterface> rei);

   private:
    std::shared_ptr<RuntimeExternalInterface> external_interface_;
    std::shared_ptr<const BinaryenWasmMemoryFactory> memory_factory_;
    std::shared_ptr<MemoryImpl> memory_;
  };

}  // namespace kagome::runtime::binaryen

#endif  // KAGOME_CORE_RUNTIME_BINARYEN_BINARYEN_MEMORY_PROVIDER_HPP
