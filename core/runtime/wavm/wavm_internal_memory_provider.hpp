/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/memory_provider.hpp"

namespace WAVM::Runtime {
  struct Memory;
}

namespace kagome::runtime::wavm {
  class MemoryImpl;

  class WavmInternalMemoryProvider final : public MemoryProvider {
   public:
    explicit WavmInternalMemoryProvider(WAVM::Runtime::Memory *memory);

    std::optional<std::reference_wrapper<runtime::Memory>> getCurrentMemory()
        const override;
    outcome::result<void> resetMemory(const MemoryConfig &) override;

   private:
    WAVM::Runtime::Memory *memory_;
    std::shared_ptr<Memory> current_memory_;
  };

}  // namespace kagome::runtime::wavm
