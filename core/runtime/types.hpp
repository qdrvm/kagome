/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RUNTIME_TYPES_HPP
#define KAGOME_RUNTIME_TYPES_HPP

#include <cstdint>
#include <utility>

namespace kagome::runtime {
  /**
   * @brief type of wasm log levels
   */
  enum class WasmLogLevel {
    Error = 0,
    Warn = 1,
    Info = 2,
    Debug = 3,
    Trace = 4,
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

  /**
   * Splits 64 bit wasm span on 32 bit pointer and 32 bit address
   */
  static constexpr std::pair<WasmPointer, WasmSize> splitSpan(WasmSpan span) {
    const auto unsigned_result = static_cast<uint64_t>(span);
    const uint32_t minor_part = unsigned_result & 0xFFFFFFFFLLU;
    const uint32_t major_part = (unsigned_result >> 32u) & 0xFFFFFFFFLLU;

    return {minor_part, major_part};
  }
}  // namespace kagome::runtime

#endif  // KAGOME_RUNTIME_TYPES_HPP
