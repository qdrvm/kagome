/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_BINARYEN_MODULE_WASM_MODULE_INSTANCE_IMPL
#define KAGOME_CORE_RUNTIME_BINARYEN_MODULE_WASM_MODULE_INSTANCE_IMPL

#include "runtime/binaryen/module/wasm_module_instance.hpp"

#include "runtime/binaryen/runtime_external_interface.hpp"

namespace wasm {
  using namespace ::wasm;  // NOLINT(google-build-using-namespace)
  class ModuleInstance;
}  // namespace wasm

namespace kagome::runtime::binaryen {

  class WasmModuleInstanceImpl final : public WasmModuleInstance {
   public:
    WasmModuleInstanceImpl(
        wasm::Module &module,
        const std::shared_ptr<RuntimeExternalInterface> &rei);

    wasm::Literal callExport(
        wasm::Name name, const std::vector<wasm::Literal> &arguments) override;

    boost::optional<wasm::Literal> getGlobal(wasm::Name name) const noexcept override;

   private:
    std::unique_ptr<wasm::ModuleInstance> module_instance_;
  };

}  // namespace kagome::runtime::binaryen

#endif  // KAGOME_CORE_RUNTIME_BINARYEN_MODULE_WASM_MODULE_INSTANCE_IMPL
