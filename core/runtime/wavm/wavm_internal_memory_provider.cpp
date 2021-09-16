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
      WAVM::Runtime::Memory* memory)
      : memory_{std::move(memory)} {
    BOOST_ASSERT(memory_ != nullptr);
  }

  boost::optional<runtime::Memory &>
  WavmInternalMemoryProvider::getCurrentMemory() const {
    return current_memory_
               ? boost::optional<runtime::Memory &>(*current_memory_)
               : boost::none;
  }

  outcome::result<void> WavmInternalMemoryProvider::resetMemory(
      WasmSize heap_base) {
    current_memory_ =
        std::make_unique<MemoryImpl>(memory_, heap_base);
    return outcome::success();
  }

}  // namespace kagome::runtime::wavm
