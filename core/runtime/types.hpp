/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <optional>
#include <utility>

#include "outcome/outcome.hpp"
#include "runtime/heap_alloc_strategy.hpp"
#include "scale/tie.hpp"

namespace kagome::runtime {
  /**
   * @brief type of wasm log levels
   */
  enum class WasmLogLevel {
    Off,
    Error,
    Warn,
    Info,
    Debug,
    Trace,
  };

  using WasmPointer = uint32_t;

  /**
   * @brief combination of pointer and size, where less significant part
   * represents wasm pointer, and most significant represents size
   */
  using WasmSpan = uint64_t;

  /**
   * @brief Size type is int32_t because we are working in 32 bit address space
   */
  using WasmSize = uint32_t;

  using WasmEnum = int32_t;

  /**
   * @brief Offset type is int32_t because we are working in 32 bit address
   * space
   */
  using WasmOffset = uint32_t;

  using WasmI32 = int32_t;
  using WasmI64 = int64_t;

  struct MemoryLimits {
    SCALE_TIE(2);

    std::optional<uint32_t> max_stack_values_num{};
    HeapAllocStrategy heap_alloc_strategy;
  };

  struct MemoryConfig {
    explicit MemoryConfig(uint32_t heap_base) : heap_base{heap_base} {}

    uint32_t heap_base;
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

  enum class Error {
    COMPILATION_FAILED = 1,
    INSTRUMENTATION_FAILED,
  };

}  // namespace kagome::runtime

OUTCOME_HPP_DECLARE_ERROR(kagome::runtime, Error);

SCALE_TIE_HASH_STD(kagome::runtime::MemoryLimits);
