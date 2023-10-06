/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <WAVM/IR/Types.h>

namespace kagome::runtime::wavm {

  /**
   * @brief Global parameters for module instantiation. Currently contains only
   * memory type that may be changed on new runtime compilation.
   *
   */
  struct ModuleParams {
    WAVM::IR::MemoryType intrinsicMemoryType{
        false, WAVM::IR::IndexType::i32, {21, UINT64_MAX}};
  };
}  // namespace kagome::runtime::wavm
