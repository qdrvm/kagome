/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_WAVM_IMPL_WAVM_MEMORY_PROVIDER_HPP
#define KAGOME_CORE_RUNTIME_WAVM_IMPL_WAVM_MEMORY_PROVIDER_HPP

#include "runtime/memory_provider.hpp"

namespace kagome::runtime::wavm {

  class IntrinsicResolver;
  class Memory;

  class WavmMemoryProvider final: public MemoryProvider {
   public:
    WavmMemoryProvider(std::shared_ptr<IntrinsicResolver> resolver);

    boost::optional<runtime::Memory&> getCurrentMemory() const override;
    void resetMemory(WasmSize heap_base) override;

   private:
    // it contains a host api module instance from which we take the memory
    std::shared_ptr<IntrinsicResolver> resolver_;
    std::unique_ptr<Memory> current_memory_;
  };

}  // namespace kagome::

#endif  // KAGOME_CORE_RUNTIME_WAVM_IMPL_WAVM_MEMORY_PROVIDER_HPP
