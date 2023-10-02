/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RUNTIME_TYPES_HPP
#define KAGOME_RUNTIME_TYPES_HPP

#include <cstdint>
#include <optional>
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

  using WasmPointer = int32_t;

  /**
   * @brief combination of pointer and size, where less significant part
   * represents wasm pointer, and most significant represents size
   */
  using WasmSpan = int64_t;

  /**
   * @brief Size type is int32_t because we are working in 32 bit address space
   */
  using WasmSize = int32_t;

  using WasmEnum = int32_t;

  /**
   * @brief Offset type is int32_t because we are working in 32 bit address
   * space
   */
  using WasmOffset = int32_t;

  using WasmI32 = int32_t;
  using WasmI64 = int64_t;

  struct MemoryLimits {
    std::optional<uint32_t> max_stack_size{};
    std::optional<uint32_t> max_stack_values_num{};
    std::optional<uint32_t> max_memory_pages_num{};
  };

  struct MemoryConfig {
    explicit MemoryConfig(uint32_t heap_base, MemoryLimits limits = {})
        : heap_base{heap_base}, limits{std::move(limits)} {}

    uint32_t heap_base;
    MemoryLimits limits;
  };

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
