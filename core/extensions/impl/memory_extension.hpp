/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MEMORY_EXTENSIONS_HPP
#define KAGOME_MEMORY_EXTENSIONS_HPP

#include "common/logger.hpp"
#include "runtime/memory.hpp"

namespace kagome::extensions {
  /**
   * Implements extension functions related to memory
   * Works with memory of wasm runtime
   */
  class MemoryExtension {
   public:
    MemoryExtension();
    explicit MemoryExtension(std::shared_ptr<runtime::Memory> memory);

    int32_t ext_malloc(uint32_t size);

    void ext_free(int32_t ptr);

   private:
    std::shared_ptr<runtime::Memory> memory_;
    common::Logger logger_;
  };
}  // namespace kagome::extensions

#endif  // KAGOME_MEMORY_EXTENSIONS_HPP
