/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/binaryen_memory_provider.hpp"

#include "runtime/common/memory_allocator.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::runtime::binaryen,
                            BinaryenMemoryProvider::Error,
                            e) {
  using E = kagome::runtime::binaryen::BinaryenMemoryProvider::Error;
  switch (e) {
    case E::OUTDATED_EXTERNAL_INTERFACE:
      return "Reference to the runtime external interface is outdated "
             "(nullptr)";
  }
  return "Unknown error in BinaryenMemoryProvider";
}

namespace kagome::runtime::binaryen {

  BinaryenMemoryProvider::BinaryenMemoryProvider(
      std::shared_ptr<const BinaryenMemoryFactory> memory_factory)
      : memory_factory_{std::move(memory_factory)} {
    BOOST_ASSERT(memory_factory_);
  }

  std::optional<std::reference_wrapper<runtime::Memory>>
  BinaryenMemoryProvider::getCurrentMemory() const {
    return memory_ == nullptr
             ? std::nullopt
             : std::optional<std::reference_wrapper<Memory>>{*memory_};
  }

  outcome::result<void> BinaryenMemoryProvider::resetMemory(
      const MemoryConfig &config) {
    auto rei = external_interface_.lock();
    BOOST_ASSERT(rei != nullptr);
    if (rei) {
      memory_ = memory_factory_->make(rei->getMemory(), config);
      return outcome::success();
    }
    return Error::OUTDATED_EXTERNAL_INTERFACE;
  }

  void BinaryenMemoryProvider::setExternalInterface(
      std::weak_ptr<RuntimeExternalInterface> rei) {
    BOOST_ASSERT(rei.lock() != nullptr);
    external_interface_ = std::move(rei);
  }

}  // namespace kagome::runtime::binaryen
