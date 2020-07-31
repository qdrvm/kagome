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

  wasm::Literal WasmModuleInstanceImpl::callExport(
      wasm::Name name, const wasm::LiteralList &arguments) {
    return module_instance_->callExport(name, arguments);
  }
}  // namespace kagome::runtime::binaryen
