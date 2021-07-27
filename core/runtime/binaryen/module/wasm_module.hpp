/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_BINARYEN_WASM_MODULE
#define KAGOME_CORE_RUNTIME_BINARYEN_WASM_MODULE

#include "runtime/binaryen/module/wasm_module_instance.hpp"
#include "runtime/binaryen/runtime_external_interface.hpp"

namespace kagome::runtime::binaryen {

  /**
   * A wrapper for binaryen's wasm::Module
   */
  class WasmModule {
   public:
    virtual ~WasmModule() = default;

    virtual std::unique_ptr<WasmModuleInstance> instantiate(
        const std::shared_ptr<RuntimeExternalInterface> &externalInterface)
        const = 0;
  };

}  // namespace kagome::runtime::binaryen

#endif  // KAGOME_CORE_RUNTIME_BINARYEN_WASM_MODULE
