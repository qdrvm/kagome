/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/wasm_executor.hpp"

#include <utility>

#include <binaryen/wasm-binary.h>
#include "runtime/binaryen/runtime_external_interface.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::runtime::binaryen, WasmExecutor::Error, e) {
  using kagome::runtime::binaryen::WasmExecutor;
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

namespace kagome::runtime::binaryen {

  WasmExecutor::WasmExecutor(std::shared_ptr<extensions::Extension> extension)
      : extension_(std::move(extension)),
        logger_{common::createLogger(kDefaultLoggerTag)} {
    BOOST_ASSERT_MSG(extension_ != nullptr, "extension is nullptr");
  }

  outcome::result<wasm::Literal> WasmExecutor::call(
      const common::Buffer &state_code,
      wasm::Name method_name,
      const wasm::LiteralList &args) {
    // that nolint supresses false positive in a library function
    // NOLINTNEXTLINE(clang-analyzer-core.NonNullParamChecker)
    if (state_code.empty()) {
      return Error::EMPTY_STATE_CODE;
    }

    wasm::Module module{};
    wasm::WasmBinaryBuilder parser(
        module,
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
    try {
      return callInModule(module, method_name, args);
    } catch (wasm::ExitException &e) {
      return Error::EXECUTION_ERROR;
    } catch (wasm::TrapException &e) {
      return Error::EXECUTION_ERROR;
    }
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

}  // namespace kagome::runtime::binaryen
