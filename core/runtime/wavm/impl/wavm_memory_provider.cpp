/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wavm/impl/wavm_memory_provider.hpp"

#include "runtime/wavm/impl/intrinsic_resolver_impl.hpp"
#include "runtime/wavm/impl/memory.hpp"

namespace kagome::runtime::wavm {

  WavmMemoryProvider::WavmMemoryProvider(
      std::shared_ptr<IntrinsicResolver> resolver)
      : resolver_{std::move(resolver)} {
    BOOST_ASSERT(resolver_);
    if (auto memory = resolver_->getMemory(); memory != nullptr) {
      current_memory_ = std::make_unique<Memory>(resolver_->getMemory());
    }
  }

  boost::optional<runtime::Memory &> WavmMemoryProvider::getCurrentMemory()
      const {
    return current_memory_
               ? boost::optional<runtime::Memory &>(*current_memory_)
               : boost::none;
  }

  void WavmMemoryProvider::resetMemory(WasmSize heap_base) {
    current_memory_ = std::make_unique<Memory>(resolver_->getMemory());
    current_memory_->setHeapBase(heap_base);
  }

}  // namespace kagome::runtime::wavm
