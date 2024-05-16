/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wavm/wavm_internal_memory_provider.hpp"

#include "runtime/common/memory_allocator.hpp"
#include "runtime/wavm/memory_impl.hpp"
#include "runtime/wavm/module_instance.hpp"

namespace kagome::runtime::wavm {

  WavmInternalMemoryProvider::WavmInternalMemoryProvider(
      WAVM::Runtime::Memory *memory)
      : memory_{memory} {
    BOOST_ASSERT(memory_ != nullptr);
  }

  std::optional<std::reference_wrapper<runtime::Memory>>
  WavmInternalMemoryProvider::getCurrentMemory() const {
    return current_memory_
             ? std::optional<std::reference_wrapper<runtime::Memory>>(
                 *current_memory_)
             : std::nullopt;
  }

  outcome::result<void> WavmInternalMemoryProvider::resetMemory(
      const MemoryConfig &config) {
    auto handle = std::make_shared<MemoryImpl>(memory_, config);
    auto allocator = std::make_unique<MemoryAllocatorImpl>(handle, config);
    current_memory_ = std::make_unique<Memory>(handle, std::move(allocator));
    return outcome::success();
  }

}  // namespace kagome::runtime::wavm
