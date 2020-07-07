/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "wasm_module_impl.hpp"

#include <memory>

#include <binaryen/wasm-binary.h>
#include <binaryen/wasm-interpreter.h>

OUTCOME_CPP_DEFINE_CATEGORY(kagome::runtime::binaryen,
                            WasmModuleImpl::Error,
                            e) {
  using Error = kagome::runtime::binaryen::WasmModuleImpl::Error;
  switch (e) {
    case Error::EMPTY_STATE_CODE:
      return "Provided state code is empty, calling a function is impossible";
    case Error::INVALID_STATE_CODE:
      return "Invalid state code, calling a function is impossible";
  }
  return "Unknown error";
}

namespace kagome::runtime::binaryen {

  WasmModuleImpl::WasmModuleImpl(
      std::unique_ptr<wasm::Module> module,
      std::unique_ptr<wasm::ModuleInstance> module_instance)
      : module_{std::move(module)},
        module_instance_{std::move(module_instance)} {
    BOOST_ASSERT(module_ != nullptr);
    BOOST_ASSERT(module_instance_ != nullptr);
  }

  outcome::result<std::unique_ptr<WasmModuleImpl>>
  WasmModuleImpl::createFromCode(
      const common::Buffer &code,
      const std::shared_ptr<RuntimeExternalInterface> &rei) {
    // that nolint supresses false positive in a library function
    // NOLINTNEXTLINE(clang-analyzer-core.NonNullParamChecker)
    if (code.empty()) {
      return Error::EMPTY_STATE_CODE;
    }

    auto module = std::make_unique<wasm::Module>();
    wasm::WasmBinaryBuilder parser(
        *module,
        reinterpret_cast<std::vector<char> const &>(  // NOLINT
            code.toVector()),
        false);

    try {
      parser.read();
    } catch (wasm::ParseException &e) {
      std::ostringstream msg;
      e.dump(msg);
      spdlog::error(msg.str());
      return Error::INVALID_STATE_CODE;
    }
    auto module_instance =
        std::make_unique<wasm::ModuleInstance>(*module, rei.get());

    std::unique_ptr<WasmModuleImpl> wasm_module_impl(
        new WasmModuleImpl(std::move(module), std::move(module_instance)));
    return wasm_module_impl;
  }

  wasm::Literal WasmModuleImpl::callExport(wasm::Name name,
                                           const wasm::LiteralList &arguments) {
    return module_instance_->callExport(name, arguments);
  }

}  // namespace kagome::runtime::binaryen
