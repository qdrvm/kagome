/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KAGOME_CORE_RUNTIME_WASM_EXECUTOR_IMPL_HPP
#define KAGOME_CORE_RUNTIME_WASM_EXECUTOR_IMPL_HPP

#include <binaryen/wasm-interpreter.h>

#include "common/buffer.hpp"
#include "common/logger.hpp"

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
      EMPTY_STATE_CODE = 1,
      INVALID_STATE_CODE,
      EXECUTION_ERROR
    };

    WasmExecutor();

    outcome::result<std::shared_ptr<wasm::Module>> prepareModule(
        const common::Buffer &state_code);

    wasm::ModuleInstance prepareModuleInstance(
        const std::shared_ptr<wasm::Module> &module,
        wasm::ModuleInstance::ExternalInterface &external_interface);

    /**
     * Executes export method from provided wasm code and returns result
     */
    outcome::result<wasm::Literal> call(
        const common::Buffer &state_code,
        wasm::ModuleInstance::ExternalInterface &external_interface,
        wasm::Name method_name,
        const wasm::LiteralList &args);

    outcome::result<wasm::Literal> call(wasm::ModuleInstance &module_instance,
                                        wasm::Name method_name,
                                        const wasm::LiteralList &args);

   private:
    common::Logger logger_;
  };

}  // namespace kagome::runtime::binaryen

OUTCOME_HPP_DECLARE_ERROR(kagome::runtime::binaryen, WasmExecutor::Error);

#endif  // KAGOME_CORE_RUNTIME_WASM_EXECUTOR_IMPL_HPP
