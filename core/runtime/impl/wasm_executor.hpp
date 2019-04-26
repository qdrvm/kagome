/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_WASM_EXECUTOR_IMPL_HPP
#define KAGOME_CORE_RUNTIME_WASM_EXECUTOR_IMPL_HPP

#include <binaryen/wasm-interpreter.h>
#include "common/buffer.hpp"
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
    explicit WasmExecutor(std::shared_ptr<extensions::Extension> extension);

    /**
     * Executes export method from provided wasm code and returns result
     */
    wasm::Literal call(const common::Buffer &state_code, wasm::Name method_name,
                       const wasm::LiteralList &args);

    /**
     * Executes export method from provided module and returns result
     */
    wasm::Literal callInModule(wasm::Module &module, wasm::Name method_name,
                               const wasm::LiteralList &args);

   private:
    std::shared_ptr<extensions::Extension> extension_;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_WASM_EXECUTOR_IMPL_HPP
