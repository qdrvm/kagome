/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/wasm_executor.hpp"

#include <utility>

#include <binaryen/shell-interface.h>
#include <binaryen/wasm-binary.h>

OUTCOME_CPP_DEFINE_CATEGORY(kagome::runtime::binaryen, WasmExecutor::Error, e) {
  using kagome::runtime::binaryen::WasmExecutor;
  switch (e) {
    case WasmExecutor::Error::EXECUTION_ERROR:
      return "An error occurred during an export call execution";
  }
}

namespace kagome::runtime::binaryen {

  outcome::result<wasm::Literal> WasmExecutor::call(
      WasmModuleInstance &module_instance,
      wasm::Name method_name,
      const std::vector<wasm::Literal> &args) {
    return module_instance.callExport(wasm::Name(method_name), args);
  }

}  // namespace kagome::runtime::binaryen
