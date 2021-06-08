/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MEMORY_EXTENSIONS_HPP
#define KAGOME_MEMORY_EXTENSIONS_HPP

#include "log/logger.hpp"
#include "runtime/types.hpp"

namespace kagome::runtime {
  class MemoryProvider;
}

namespace kagome::host_api {
  /**
   * Implements extension functions related to memory
   * Works with memory of wasm runtime
   */
  class MemoryExtension {
   public:
    explicit MemoryExtension(std::shared_ptr<const runtime::MemoryProvider> memory_provider);

    void reset();

    // ----------------- memory api v1 -----------------
    /**
     * @see Extension::ext_allocator_malloc_version_1
     */
    runtime::WasmPointer ext_allocator_malloc_version_1(runtime::WasmSize size);

    /**
     * @see Extension::ext_allocator_free_version_1
     */
    void ext_allocator_free_version_1(runtime::WasmPointer ptr);

   private:
    std::shared_ptr<const runtime::MemoryProvider> memory_provider_;
    log::Logger logger_;
  };
}  // namespace kagome::host_api

#endif  // KAGOME_MEMORY_EXTENSIONS_HPP
