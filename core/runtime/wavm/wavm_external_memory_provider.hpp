/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/memory_provider.hpp"

namespace kagome::runtime::wavm {

  class IntrinsicModuleInstance;
  class MemoryImpl;

  class WavmExternalMemoryProvider final : public MemoryProvider {
   public:
    explicit WavmExternalMemoryProvider(
        std::shared_ptr<IntrinsicModuleInstance> intrinsic_module);

    std::optional<std::reference_wrapper<runtime::Memory>> getCurrentMemory()
        const override;
    outcome::result<void> resetMemory(const MemoryConfig &) override;

   private:
    // it contains the memory itself
    std::shared_ptr<IntrinsicModuleInstance> intrinsic_module_;
    std::shared_ptr<Memory> current_memory_;
  };

}  // namespace kagome::runtime::wavm
