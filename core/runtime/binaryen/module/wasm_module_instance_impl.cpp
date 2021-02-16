/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/module/wasm_module_instance_impl.hpp"

#include <binaryen/wasm.h>

namespace kagome::runtime::binaryen {

  WasmModuleInstanceImpl::WasmModuleInstanceImpl(
      wasm::Module &module,
      const std::shared_ptr<RuntimeExternalInterface> &rei)
      : module_instance_{
          std::make_unique<wasm::ModuleInstance>(module, rei.get())} {
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
