/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/impl/wasm_executor.hpp"

#include <binaryen/shell-interface.h>
#include <binaryen/wasm-binary.h>

#include <utility>

OUTCOME_CPP_DEFINE_CATEGORY(kagome::runtime, WasmExecutor::Error, e) {
  using kagome::runtime::WasmExecutor;
  switch (e) {
    case WasmExecutor::Error::EMPTY_STATE_CODE:
      return "Provided state code is empty, calling a function is impossible";
    case WasmExecutor::Error::INVALID_STATE_CODE:
      return "Invalid state code, calling a function is impossible";
    case WasmExecutor::Error::EXECUTION_ERROR:
      return "An error occured during call execution";
  }
  return "Unknown error";
}

namespace kagome::runtime {

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

}  // namespace kagome::runtime
