/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_MEMORY_PROVIDER_HPP
#define KAGOME_CORE_RUNTIME_MEMORY_PROVIDER_HPP

#include <optional>

#include "runtime/memory.hpp"
#include "runtime/common/memory_allocator.hpp"

namespace kagome::runtime {

  class Memory;

  class MemoryProvider {
   public:
    virtual ~MemoryProvider() = default;

    virtual std::optional<std::reference_wrapper<runtime::Memory>>
    getCurrentMemory() const = 0;

    [[nodiscard]] virtual outcome::result<void> resetMemory(
        const MemoryConfig&) = 0;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_MEMORY_PROVIDER_HPP
