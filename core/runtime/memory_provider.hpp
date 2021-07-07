/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_MEMORY_PROVIDER_HPP
#define KAGOME_CORE_RUNTIME_MEMORY_PROVIDER_HPP

#include <boost/optional.hpp>

#include "runtime/memory.hpp"

namespace kagome::runtime {

  class Memory;

  class MemoryProvider {
   public:
    virtual ~MemoryProvider() = default;

    virtual boost::optional<runtime::Memory&> getCurrentMemory() const = 0;
    virtual void resetMemory(WasmSize heap_base) = 0;

  };

}

#endif  // KAGOME_CORE_RUNTIME_MEMORY_PROVIDER_HPP
