/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/wasm_executor.hpp"

#include <binaryen/shell-interface.h>

OUTCOME_CPP_DEFINE_CATEGORY(kagome::runtime::binaryen, WasmExecutor::Error, e) {
  using kagome::runtime::binaryen::WasmExecutor;
  switch (e) {
    case WasmExecutor::Error::UNEXPECTED_EXIT:
      return "Execution was ended in external function";
    case WasmExecutor::Error::EXECUTION_ERROR:
      return "An error occurred during an export call execution";
    case WasmExecutor::Error::CAN_NOT_OBTAIN_GLOBAL:
      return "Failed to obtain a global value";
  }
  return "Unknown WasmExecutor error";
}

namespace kagome::runtime::binaryen {

  outcome::result<wasm::Literal> WasmExecutor::call(
      WasmModuleInstance &module_instance,
      wasm::Name method_name,
      const std::vector<wasm::Literal> &args) {
    try {
      return module_instance.callExportFunction(wasm::Name(method_name), args);
    } catch (wasm::ExitException &e) {
      return Error::UNEXPECTED_EXIT;
    } catch (wasm::TrapException &e) {
      return Error::EXECUTION_ERROR;
    }
  }

  outcome::result<wasm::Literal> WasmExecutor::get(
      WasmModuleInstance &module_instance, wasm::Name global_name) {
    try {
      return module_instance.getExportGlobal(wasm::Name(global_name));
    } catch (wasm::TrapException &e) {
      return Error::CAN_NOT_OBTAIN_GLOBAL;
    }
  }

}  // namespace kagome::runtime::binaryen
