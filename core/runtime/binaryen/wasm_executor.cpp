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

#include "runtime/binaryen/wasm_executor.hpp"

#include <utility>

#include <binaryen/shell-interface.h>
#include <binaryen/wasm-binary.h>

OUTCOME_CPP_DEFINE_CATEGORY(kagome::runtime::binaryen, WasmExecutor::Error, e) {
  using kagome::runtime::binaryen::WasmExecutor;
  switch (e) {
    case WasmExecutor::Error::EMPTY_STATE_CODE:
      return "Provided state code is empty, calling a function is impossible";
    case WasmExecutor::Error::INVALID_STATE_CODE:
      return "Invalid state code, calling a function is impossible";
    case WasmExecutor::Error::EXECUTION_ERROR:
      return "An error occurred during an export call execution";
  }
  return "Unknown error";
}

namespace kagome::runtime::binaryen {

  WasmExecutor::WasmExecutor()
      : logger_{common::createLogger("Wasm executor")} {}

  outcome::result<std::shared_ptr<wasm::Module>> WasmExecutor::prepareModule(
      const common::Buffer &state_code) {
    // that nolint supresses false positive in a library function
    // NOLINTNEXTLINE(clang-analyzer-core.NonNullParamChecker)
    if (state_code.empty()) {
      return Error::EMPTY_STATE_CODE;
    }

    auto module = std::make_shared<wasm::Module>();
    wasm::WasmBinaryBuilder parser(
        *module,
        reinterpret_cast<std::vector<char> const &>(  // NOLINT
            state_code.toVector()),
        false);
    try {
      parser.read();
    } catch (wasm::ParseException &e) {
      std::ostringstream msg;
      e.dump(msg);
      logger_->error(msg.str());
      return Error::INVALID_STATE_CODE;
    }
    return module;
  }

  wasm::ModuleInstance WasmExecutor::prepareModuleInstance(
      const std::shared_ptr<wasm::Module> &module,
      wasm::ModuleInstance::ExternalInterface &external_interface) {
    return wasm::ModuleInstance(*module, &external_interface);
  }

  outcome::result<wasm::Literal> WasmExecutor::call(
      const common::Buffer &state_code,
      wasm::ModuleInstance::ExternalInterface &external_interface,
      wasm::Name method_name,
      const wasm::LiteralList &args) {
    OUTCOME_TRY(module, prepareModule(state_code));
    auto module_instance = prepareModuleInstance(module, external_interface);
    return call(module_instance, method_name, args);
  }

  outcome::result<wasm::Literal> WasmExecutor::call(
      wasm::ModuleInstance &module_instance,
      wasm::Name method_name,
      const wasm::LiteralList &args) {
    try {
      return module_instance.callExport(wasm::Name(method_name), args);
    } catch (wasm::ExitException &e) {
      return Error::EXECUTION_ERROR;
    } catch (wasm::TrapException &e) {
      return Error::EXECUTION_ERROR;
    }
  }

}  // namespace kagome::runtime::binaryen
