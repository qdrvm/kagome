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

#ifndef KAGOME_IO_EXTENSION_HPP
#define KAGOME_IO_EXTENSION_HPP

#include <cstdint>

#include "common/logger.hpp"
#include "runtime/wasm_memory.hpp"

namespace kagome::extensions {
  /**
   * Implements extension functions related to IO
   */
  class IOExtension {
   public:
    explicit IOExtension(std::shared_ptr<runtime::WasmMemory> memory);

    /**
     * @see Extension::ext_print_hex
     */
    void ext_print_hex(runtime::WasmPointer data, runtime::SizeType length);

    /**
     * @see Extension::ext_print_num
     */
    void ext_print_num(uint64_t value);

    /**
     * @see Extension::ext_print_utf8
     */
    void ext_print_utf8(runtime::WasmPointer utf8_data,
                        runtime::SizeType utf8_length);

   private:
    constexpr static auto kDefaultLoggerTag = "WASM Runtime [IOExtension]";
    std::shared_ptr<runtime::WasmMemory> memory_;
    common::Logger logger_;
  };
}  // namespace kagome::extensions

#endif  // KAGOME_IO_EXTENSION_HPP
