/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
    runtime::WasmPointer ext_malloc(runtime::SizeType size);

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
