/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/binaryen/memory_impl.hpp"
#include "runtime/binaryen/runtime_external_interface.hpp"

namespace kagome::runtime::binaryen {

  class BinaryenMemoryFactory {
   public:
    virtual ~BinaryenMemoryFactory() = default;

    virtual std::unique_ptr<MemoryImpl> make(
        RuntimeExternalInterface::InternalMemory *memory,
        const MemoryConfig &config) const;
  };

}  // namespace kagome::runtime::binaryen
