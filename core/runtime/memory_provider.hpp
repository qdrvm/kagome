/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <optional>

#include "runtime/common/memory_allocator.hpp"
#include "runtime/memory.hpp"

namespace kagome::runtime {

  class Memory;

  class MemoryProvider {
   public:
    virtual ~MemoryProvider() = default;

    virtual std::optional<std::reference_wrapper<runtime::Memory>>
    getCurrentMemory() const = 0;

    [[nodiscard]] virtual outcome::result<void> resetMemory(
        const MemoryConfig &) = 0;
  };

}  // namespace kagome::runtime
