/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/impl/wasm_executor.hpp"

#include <binaryen/wasm-binary.h>
#include "runtime/impl/runtime_external_interface.hpp"

namespace kagome::runtime {

  WasmExecutor::WasmExecutor(std::shared_ptr<extensions::Extension> extension)
      : extension_(std::move(extension)) {}

  wasm::Literal WasmExecutor::call(const common::Buffer &state_code,
                                   wasm::Name method_name,
                                   const wasm::LiteralList &args) {
    wasm::Module module{};
    wasm::WasmBinaryBuilder parser(
        module,
        reinterpret_cast<std::vector<char> const &>(  // NOLINT
            state_code.toVector()),
        false);
    parser.read();

    return callInModule(module, method_name, args);
  }

  wasm::Literal WasmExecutor::callInModule(wasm::Module &module,
                                           wasm::Name method_name,
                                           const wasm::LiteralList &args) {
    // prepare external interface with extern methods
    RuntimeExternalInterface rei(extension_);

    // interpret module
    wasm::ModuleInstance module_instance(module, &rei);

    return module_instance.callExport(wasm::Name(method_name), args);
  }

}  // namespace kagome::runtime
