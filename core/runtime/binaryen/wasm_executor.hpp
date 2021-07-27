/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_WASM_EXECUTOR_IMPL_HPP
#define KAGOME_CORE_RUNTIME_WASM_EXECUTOR_IMPL_HPP

#include "common/buffer.hpp"
#include "log/logger.hpp"
#include "runtime/binaryen/module/wasm_module_instance.hpp"

namespace kagome::runtime::binaryen {

  /**
   * @brief WasmExecutor is the helper to execute export functions from wasm
   * runtime
   * @note This class is implementation detail and should never be used outside
   * this directory
   */
  class WasmExecutor {
   public:
    enum class Error {
      UNEXPECTED_EXIT = 1,
      EXECUTION_ERROR,
      CAN_NOT_OBTAIN_GLOBAL
    };

    outcome::result<wasm::Literal> call(WasmModuleInstance &module_instance,
                                        wasm::Name method_name,
                                        const std::vector<wasm::Literal> &args);

    outcome::result<wasm::Literal> get(WasmModuleInstance &module_instance,
                                       wasm::Name global_name);
  };

}  // namespace kagome::runtime::binaryen

OUTCOME_HPP_DECLARE_ERROR(kagome::runtime::binaryen, WasmExecutor::Error);

#endif  // KAGOME_CORE_RUNTIME_WASM_EXECUTOR_IMPL_HPP
