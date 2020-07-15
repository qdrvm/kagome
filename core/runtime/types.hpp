/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RUNTIME_TYPES_HPP
#define KAGOME_RUNTIME_TYPES_HPP

#include <cstdint>

namespace kagome::runtime {
  /**
   * @brief type of wasm log levels
   */
  enum WasmLogLevel {
    WasmLL_Error = 0,
    WasmLL_Warn = 1,
    WasmLL_Info = 2,
    WasmLL_Debug = 3,
    WasmLL_Trace = 4,
  };
  /**
   * @brief type of wasm memory is 32 bit integer
   */
  using WasmPointer = uint32_t;
  /**
   * @brief combination of pointer and size, where less significant part
   * represents wasm pointer, and most significant represents size
   */
  using WasmSpan = uint64_t;
  /**
   * @brief Size type is uint32_t because we are working in 32 bit address space
   */
  using WasmSize = uint32_t;
  /**
   * @brief Enum value is uint32_t
   */
  using WasmEnum = uint32_t;
  /**
   * @brief Offset type is uint32_t because we are working in 32 bit address
   * space
   */
  using WasmOffset = uint32_t;
}  // namespace kagome::runtime

#endif  // KAGOME_RUNTIME_TYPES_HPP
