/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
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
    void ext_print_hex(runtime::WasmPointer data, runtime::WasmSize length);

    /**
     * @see Extension::ext_print_num
     */
    void ext_print_num(uint64_t value);

    /**
     * @see Extension::ext_print_utf8
     */
    void ext_print_utf8(runtime::WasmPointer utf8_data,
                        runtime::WasmSize utf8_length);

   private:
    constexpr static auto kDefaultLoggerTag = "WASM Runtime [IOExtension]";
    std::shared_ptr<runtime::WasmMemory> memory_;
    common::Logger logger_;
  };
}  // namespace kagome::extensions

#endif  // KAGOME_IO_EXTENSION_HPP
