/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_BINARYEN_MODULE_WASM_MODULE_INSTANCE_IMPL
#define KAGOME_CORE_RUNTIME_BINARYEN_MODULE_WASM_MODULE_INSTANCE_IMPL

#include "runtime/binaryen/module/wasm_module_instance.hpp"

namespace wasm {
  using namespace ::wasm;  // NOLINT(google-build-using-namespace)
  class Module;
  class ModuleInstance;
}  // namespace wasm

namespace kagome::runtime::binaryen {

  class RuntimeExternalInterface;

  class WasmModuleInstanceImpl final : public WasmModuleInstance {
   public:
    WasmModuleInstanceImpl(
        std::shared_ptr<wasm::Module> parent,
        const std::shared_ptr<RuntimeExternalInterface> &rei);

    wasm::Literal callExportFunction(
        wasm::Name name, const std::vector<wasm::Literal> &arguments) override;

    wasm::Literal getExportGlobal(wasm::Name name) override;

   private:
    std::shared_ptr<wasm::Module>
        parent_;  // must be kept alive because binaryen's module instance keeps
                  // a reference to it
    std::unique_ptr<wasm::ModuleInstance> module_instance_;
  };

}  // namespace kagome::runtime::binaryen

#endif  // KAGOME_CORE_RUNTIME_BINARYEN_MODULE_WASM_MODULE_INSTANCE_IMPL
