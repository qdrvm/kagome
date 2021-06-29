/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/module/wasm_module_instance_impl.hpp"

#include <binaryen/wasm.h>
#include <binaryen/wasm-interpreter.h>

#include "runtime/binaryen/runtime_external_interface.hpp"

namespace kagome::runtime::binaryen {

  WasmModuleInstanceImpl::WasmModuleInstanceImpl(
      std::shared_ptr<wasm::Module> parent,
      const std::shared_ptr<RuntimeExternalInterface> &rei)
      : parent_{std::move(parent)},
        module_instance_{
            std::make_unique<wasm::ModuleInstance>(*parent_, rei.get())} {
    BOOST_ASSERT(parent_);
    BOOST_ASSERT(module_instance_);
  }

  wasm::Literal WasmModuleInstanceImpl::callExportFunction(
      wasm::Name name, const wasm::LiteralList &arguments) {
    return module_instance_->callExport(name, arguments);
  }

  wasm::Literal WasmModuleInstanceImpl::getExportGlobal(wasm::Name name) {
    return module_instance_->getExport(name);
  }

}  // namespace kagome::runtime::binaryen
