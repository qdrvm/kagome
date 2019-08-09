/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RUNTIME_COMMON_HPP
#define KAGOME_RUNTIME_COMMON_HPP

#include <cstdint>

namespace kagome::runtime {
  /**
   * @brief type of wasm memory is 32 bit integer
   */
  using WasmPointer = int32_t;
  /**
   * @brief Size type is uint32_t because we are working in 32 bit address space
   */
  using SizeType = uint32_t;
}  // namespace kagome::runtime

#endif  // KAGOME_RUNTIME_COMMON_HPP
