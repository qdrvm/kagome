/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wavm/impl/wavm_memory_provider.hpp"

#include "runtime/wavm/impl/intrinsic_module_instance.hpp"
#include "runtime/wavm/impl/memory.hpp"

namespace kagome::runtime::wavm {

  WavmMemoryProvider::WavmMemoryProvider(
      std::shared_ptr<IntrinsicModuleInstance> module)
      : intrinsic_module_{std::move(module)} {
    BOOST_ASSERT(intrinsic_module_);
    if (auto memory = intrinsic_module_->getExportedMemory();
        memory != nullptr) {
      current_memory_ =
          std::make_unique<Memory>(intrinsic_module_->getExportedMemory());
    }
  }

  boost::optional<runtime::Memory &> WavmMemoryProvider::getCurrentMemory()
      const {
    return current_memory_
               ? boost::optional<runtime::Memory &>(*current_memory_)
               : boost::none;
  }

  void WavmMemoryProvider::resetMemory(WasmSize heap_base) {
    current_memory_ =
        std::make_unique<Memory>(intrinsic_module_->getExportedMemory());
    current_memory_->setHeapBase(heap_base);
  }

}  // namespace kagome::runtime::wavm
