/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/binaryen_memory_provider.hpp"

#include "runtime/common/memory_allocator.hpp"

namespace kagome::runtime::binaryen {
  BinaryenMemoryProvider::BinaryenMemoryProvider(
      std::shared_ptr<const BinaryenWasmMemoryFactory> memory_factory)
      : memory_factory_{std::move(memory_factory)} {
    BOOST_ASSERT(memory_factory_);
  }

  boost::optional<runtime::Memory &> BinaryenMemoryProvider::getCurrentMemory()
      const {
    return memory_ == nullptr ? boost::none
                              : boost::optional<Memory &>{*memory_};
  }

  void BinaryenMemoryProvider::resetMemory(WasmSize heap_base) {
    BOOST_ASSERT(external_interface_ != nullptr);
    memory_ =
        memory_factory_->make(external_interface_->getMemory(), heap_base);
  }

  void BinaryenMemoryProvider::setExternalInterface(
      std::shared_ptr<RuntimeExternalInterface> rei) {
    BOOST_ASSERT(rei != nullptr);
    external_interface_ = rei;
  }

}  // namespace kagome::runtime::binaryen
