/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_BINARYEN_WASM_MODULE_IMPL
#define KAGOME_CORE_RUNTIME_BINARYEN_WASM_MODULE_IMPL

#include "common/buffer.hpp"
#include "runtime/binaryen/runtime_external_interface.hpp"
#include "wasm_module.hpp"

namespace wasm {
  using namespace ::wasm;
  class Module;
  class ModuleInstance;
}  // namespace wasm

namespace kagome::runtime::binaryen {

  class WasmModuleImpl final : public WasmModule {
   public:
    enum class Error { EMPTY_STATE_CODE = 1, INVALID_STATE_CODE };

    static outcome::result<std::unique_ptr<WasmModuleImpl>> createFromCode(
        const common::Buffer &code, std::shared_ptr<RuntimeExternalInterface> rei);

    wasm::Literal callExport(
        wasm::Name name, const std::vector<wasm::Literal> &arguments) override;

   private:
    explicit WasmModuleImpl(
        std::unique_ptr<wasm::Module> module,
        std::unique_ptr<wasm::ModuleInstance> module_instance);

    std::unique_ptr<wasm::Module> module_;
    std::unique_ptr<wasm::ModuleInstance> module_instance_;
  };


}  // namespace kagome::runtime::binaryen

OUTCOME_HPP_DECLARE_ERROR(kagome::runtime::binaryen, WasmModuleImpl::Error);

#endif  // KAGOME_CORE_RUNTIME_BINARYEN_WASM_MODULE_IMPL
