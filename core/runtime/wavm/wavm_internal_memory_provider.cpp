/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
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
      const MemoryConfig& config) {
    current_memory_ = std::make_unique<MemoryImpl>(memory_, config);
    return outcome::success();
  }

}  // namespace kagome::runtime::wavm
