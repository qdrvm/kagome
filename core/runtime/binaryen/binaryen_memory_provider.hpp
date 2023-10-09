/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/binaryen/binaryen_memory_factory.hpp"
#include "runtime/binaryen/binaryen_memory_provider.hpp"
#include "runtime/binaryen/runtime_external_interface.hpp"
#include "runtime/memory_provider.hpp"

namespace wasm {
  class RuntimeExternalInterface;
}

namespace kagome::runtime::binaryen {

  class BinaryenMemoryProvider final : public MemoryProvider {
   public:
    enum class Error { OUTDATED_EXTERNAL_INTERFACE = 1 };

    BinaryenMemoryProvider(
        std::shared_ptr<const BinaryenMemoryFactory> memory_factory);

    std::optional<std::reference_wrapper<runtime::Memory>> getCurrentMemory()
        const override;

    [[nodiscard]] outcome::result<void> resetMemory(
        const MemoryConfig &config) override;

    void setExternalInterface(std::weak_ptr<RuntimeExternalInterface> rei);

   private:
    std::weak_ptr<RuntimeExternalInterface> external_interface_;
    std::shared_ptr<const BinaryenMemoryFactory> memory_factory_;
    std::shared_ptr<MemoryImpl> memory_;
  };

}  // namespace kagome::runtime::binaryen

OUTCOME_HPP_DECLARE_ERROR(kagome::runtime::binaryen,
                          BinaryenMemoryProvider::Error);
