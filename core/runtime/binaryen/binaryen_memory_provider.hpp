/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_BINARYEN_BINARYEN_MEMORY_PROVIDER_HPP
#define KAGOME_CORE_RUNTIME_BINARYEN_BINARYEN_MEMORY_PROVIDER_HPP

#include "runtime/binaryen/binaryen_memory_provider.hpp"
#include "runtime/binaryen/binaryen_wasm_memory_factory.hpp"
#include "runtime/memory_provider.hpp"

namespace wasm {
  class ShellExternalInterface;
}

namespace kagome::runtime::binaryen {

  class BinaryenMemoryProvider final : public MemoryProvider {
   public:
    BinaryenMemoryProvider(
        std::unique_ptr<BinaryenWasmMemoryFactory> memory_factory)
        : memory_factory_{std::move(memory_factory)} {
      BOOST_ASSERT(memory_factory_);
    }

    boost::optional<std::shared_ptr<Memory>> getCurrentMemory() const override {
      return memory_ == nullptr ? boost::none
                                : boost::optional<std::shared_ptr<Memory>>{
                                    std::static_pointer_cast<Memory>(memory_)};
    }

    void resetMemory(WasmSize heap_base) override {
      memory_->reset();
      memory_->setHeapBase(heap_base);
    }

    void setMemory(wasm::ShellExternalInterface::Memory *memory) {
      BOOST_ASSERT(memory != nullptr);
      memory_ = memory_factory_->make(memory);
    }

   private:
    std::shared_ptr<BinaryenWasmMemoryFactory> memory_factory_;
    std::shared_ptr<WasmMemoryImpl> memory_;
  };

}  // namespace kagome::runtime::binaryen

#endif  // KAGOME_CORE_RUNTIME_BINARYEN_BINARYEN_MEMORY_PROVIDER_HPP
