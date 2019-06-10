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
  class WasmResult {
   public:
    explicit constexpr WasmResult(int64_t v)
        : address_(getWasmAddr(v)), length_(getWasmLen(v)) {}

    /**
     * @arg runtime_call_result is a wasm literal which is the result of
     * execution of a wasm function in Runtime API
     * @returns the address of the buffer returned by the function
     */
    inline constexpr WasmPointer address() {
      return address_;
    }

    /**
     * @param runtime_call_result the result of execution of a wasm function
     * in Runtime API
     * @return result length
     */
    inline constexpr SizeType length() {
      return length_;
    }

   private:
    inline constexpr WasmPointer getWasmAddr(int64_t runtime_call_result) {
      auto unsigned_result = static_cast<uint64_t>(runtime_call_result);
      uint64_t minor_part = unsigned_result & 0xFFFFFFFFLLU;
      return static_cast<WasmPointer>(minor_part);
    }

    inline constexpr SizeType getWasmLen(int64_t runtime_call_result) {
      auto unsigned_result = static_cast<uint64_t>(runtime_call_result);
      uint64_t major_part = (unsigned_result >> 32u) & 0xFFFFFFFFLLU;
      return static_cast<SizeType>(major_part);
    }

    WasmPointer address_;
    SizeType length_;
  };
}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_WASM_RESULT_HPP
