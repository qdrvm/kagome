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
      std::shared_ptr<IntrinsicModuleInstance> module)
      : intrinsic_module_{std::move(module)} {
    BOOST_ASSERT(intrinsic_module_ != nullptr);
  }

  std::optional<std::reference_wrapper<runtime::Memory>>
  WavmExternalMemoryProvider::getCurrentMemory() const {
    return current_memory_
             ? std::optional<std::reference_wrapper<runtime::Memory>>(
                 *current_memory_)
             : std::nullopt;
  }

  outcome::result<void> WavmExternalMemoryProvider::resetMemory(
      const MemoryConfig &config) {
    current_memory_ = std::make_unique<MemoryImpl>(
        intrinsic_module_->getExportedMemory(), config);
    return outcome::success();
  }

}  // namespace kagome::runtime::wavm
