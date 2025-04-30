/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <optional>
#include <utility>

#include "runtime/heap_alloc_strategy.hpp"

namespace kagome::runtime {
  /**
   * @brief type of wasm log levels
   */
  enum class WasmLogLevel : uint8_t {
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
    std::optional<uint32_t> max_stack_values_num;
    HeapAllocStrategy heap_alloc_strategy;
    bool operator==(const MemoryLimits &) const = default;
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

  enum class Error : uint8_t {
    COMPILATION_FAILED = 1,
    INSTRUMENTATION_FAILED,
  };

  enum class OptimizationLevel : uint8_t {
    O0,
    O1,
    O2,
  };

  constexpr std::string_view to_string(OptimizationLevel lvl) {
    switch (lvl) {
      case OptimizationLevel::O0:
        return "O0";
      case OptimizationLevel::O1:
        return "O1";
      case OptimizationLevel::O2:
        return "O2";
    }
    UNREACHABLE
  }

  // O2 is temporarily not default because there is a runtime on Polkadot 
  // that compiles for an indefinite amount of time on O2
  static constexpr OptimizationLevel DEFAULT_RELAY_CHAIN_RUNTIME_OPT_LEVEL =
      OptimizationLevel::O1;

}  // namespace kagome::runtime

OUTCOME_HPP_DECLARE_ERROR(kagome::runtime, Error);

SCALE_TIE_HASH_STD(kagome::runtime::MemoryLimits);
