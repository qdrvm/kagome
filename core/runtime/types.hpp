/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RUNTIME_TYPES_HPP
#define KAGOME_RUNTIME_TYPES_HPP

#include <cstdint>

namespace kagome::runtime {
  /**
   * @brief type of wasm memory is 32 bit integer
   */
  using WasmPointer = uint32_t;
  /**
   * @brief combination of pointer and size, where less significant part
   * represents wasm pointer, and most significant represents size
   */
  using PointerSize = uint64_t;
  /**
   * @brief Size type is uint32_t because we are working in 32 bit address space
   */
  using SizeType = uint32_t;

  constexpr WasmPointer kNullWasmPointer = 0u;
  constexpr PointerSize kNullPointerSize = 0u;
  constexpr SizeType kNullSizeType = 0u;

  inline PointerSize makePointerSize(WasmPointer pointer, SizeType size) {
    return static_cast<uint64_t>(pointer)
           | (static_cast<uint64_t>(size) << 32ul);
  }

  /**
   * @brief result of splitting pointer-size
   */
  struct PointerSizeStructure {
    WasmPointer pointer;
    SizeType size;
  };

  /**
   * @brief splits pointer-size into pointer and size
   * @param pointer_size pointer-size value
   * @return pointer and size
   */
  inline PointerSizeStructure splitPointerSize(PointerSize pointer_size) {
    WasmPointer pointer = pointer_size & 0xFFFFFFFF;
    SizeType size = static_cast<uint32_t>(pointer_size >> 32ul);
    return {pointer, size};
  }
}  // namespace kagome::runtime

#endif  // KAGOME_RUNTIME_TYPES_HPP
