/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wavm/wavm_external_memory_provider.hpp"

#include "runtime/common/memory_allocator.hpp"
#include "runtime/wavm/intrinsics/intrinsic_module_instance.hpp"
#include "runtime/wavm/memory_impl.hpp"

namespace kagome::runtime::wavm {

  WavmExternalMemoryProvider::WavmExternalMemoryProvider(
      std::weak_ptr<IntrinsicModuleInstance> module)
      : intrinsic_module_{std::move(module)} {
    BOOST_ASSERT(intrinsic_module_.lock() != nullptr);
  }

  boost::optional<runtime::Memory &>
  WavmExternalMemoryProvider::getCurrentMemory() const {
    return current_memory_
               ? boost::optional<runtime::Memory &>(*current_memory_)
               : boost::none;
  }

  outcome::result<void> WavmExternalMemoryProvider::resetMemory(
      WasmSize heap_base) {
    BOOST_ASSERT(intrinsic_module_.lock() != nullptr);
    current_memory_ = std::make_unique<MemoryImpl>(
        intrinsic_module_.lock()->getExportedMemory(), heap_base);
    return outcome::success();
  }

}  // namespace kagome::runtime::wavm
