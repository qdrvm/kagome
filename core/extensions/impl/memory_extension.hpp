/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MEMORY_EXTENSIONS_HPP
#define KAGOME_MEMORY_EXTENSIONS_HPP

#include "common/logger.hpp"
#include "runtime/wasm_memory.hpp"

namespace kagome::extensions {
  /**
   * Implements extension functions related to memory
   * Works with memory of wasm runtime
   */
  class MemoryExtension {
   public:
    explicit MemoryExtension(std::shared_ptr<runtime::WasmMemory> memory);

    /**
     * @see Extension::ext_malloc
     */
    runtime::WasmPointer ext_malloc(runtime::WasmSize size);

    /**
     * @see Extension::ext_free
     */
    void ext_free(runtime::WasmPointer ptr);

   private:
    constexpr static auto kDefaultLoggerTag = "WASM Runtime [MemoryExtension]";
    std::shared_ptr<runtime::WasmMemory> memory_;
    common::Logger logger_;
  };
}  // namespace kagome::extensions

#endif  // KAGOME_MEMORY_EXTENSIONS_HPP
