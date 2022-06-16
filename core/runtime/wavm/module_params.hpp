/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_WAVM_MODULE_PARAMS_HPP
#define KAGOME_CORE_RUNTIME_WAVM_MODULE_PARAMS_HPP

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

#endif  // KAGOME_CORE_RUNTIME_WAVM_MODULE_PARAMS_HPP
