/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_WASM_EXECUTOR_IMPL_HPP
#define KAGOME_CORE_RUNTIME_WASM_EXECUTOR_IMPL_HPP

#include <binaryen/wasm-interpreter.h>
#include "common/buffer.hpp"
#include "common/logger.hpp"
#include "extensions/extension.hpp"

namespace kagome::runtime {

  /**
   * @brief WasmExecutor is the helper to execute export functions from wasm
   * runtime
   * @note This class is implementation detail and should never be used outside
   * this directory
   */
  class WasmExecutor {
   public:
    enum class Error {
      EMPTY_STATE_CODE = 1,
      INVALID_STATE_CODE,
      EXECUTION_ERROR
    };

    explicit WasmExecutor(std::shared_ptr<extensions::Extension> extension);

    /**
     * Executes export method from provided wasm code and returns result
     */
    outcome::result<wasm::Literal> call(const common::Buffer &state_code,
                                        wasm::Name method_name,
                                        const wasm::LiteralList &args);

    /**
     * Executes export method from provided module and returns result
     */
    wasm::Literal callInModule(wasm::Module &module,
                               wasm::Name method_name,
                               const wasm::LiteralList &args);

   private:
    constexpr static auto kDefaultLoggerTag = "Wasm executor";

    std::shared_ptr<extensions::Extension> extension_;
    common::Logger logger_;
  };

}  // namespace kagome::runtime

OUTCOME_HPP_DECLARE_ERROR(kagome::runtime, WasmExecutor::Error);

#endif  // KAGOME_CORE_RUNTIME_WASM_EXECUTOR_IMPL_HPP
