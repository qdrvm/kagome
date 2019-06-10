/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_WASM_RESULT_HPP
#define KAGOME_CORE_RUNTIME_WASM_RESULT_HPP

#include "runtime/common.hpp"

namespace kagome::runtime {
  /**
   * Result of a call to a Runtime API wasm function is an i64 where first 32
   * bits are the address and next 32 bits are the size of the returned buffer.
   */
  struct WasmResult {
    explicit constexpr WasmResult(int64_t v)
        : address(getWasmAddr(v)), length(getWasmLen(v)) {}

    WasmPointer address = 0u;  ///< address of buffer result
    SizeType length = 0u;      ///< length of buffer result

   private:
    /**
     * @arg runtime_call_result is a wasm literal which is the result of
     * execution of a wasm function in Runtime API
     * @returns the address of the buffer returned by the function
     */
    inline constexpr WasmPointer getWasmAddr(int64_t runtime_call_result) {
      auto unsigned_result = static_cast<uint64_t>(runtime_call_result);
      uint64_t minor_part = unsigned_result & 0xFFFFFFFFLLU;
      return static_cast<WasmPointer>(minor_part);
    }

    /**
     * @param runtime_call_result the result of execution of a wasm function
     * in Runtime API
     * @return result length
     */
    inline constexpr SizeType getWasmLen(int64_t runtime_call_result) {
      auto unsigned_result = static_cast<uint64_t>(runtime_call_result);
      uint64_t major_part = (unsigned_result >> 32u) & 0xFFFFFFFFLLU;
      return static_cast<SizeType>(major_part);
    }
  };
}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_WASM_RESULT_HPP
