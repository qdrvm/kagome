/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_BINARYEN_MODULE_WASM_MODULE_FACTORY
#define KAGOME_CORE_RUNTIME_BINARYEN_MODULE_WASM_MODULE_FACTORY

#include "common/buffer.hpp"
#include "outcome/outcome.hpp"
#include "runtime/binaryen/module/wasm_module.hpp"
#include "runtime/binaryen/runtime_external_interface.hpp"

namespace kagome::runtime::binaryen {

  /**
   * An abstract factory to produce WasmModules
   */
  class WasmModuleFactory {
   public:
    virtual ~WasmModuleFactory() = default;

    /**
     * A module will be created with the provided \arg code and instantiated
     * with \arg rei (runtime external interface)
     * \arg storage_provider is required to read runtime properties from state
     * @return the module in case of success
     */
    virtual outcome::result<std::unique_ptr<WasmModule>> createModule(
        const common::Buffer &code,
        std::shared_ptr<RuntimeExternalInterface> rei,
        std::shared_ptr<TrieStorageProvider> storage_provider) const = 0;
  };

}  // namespace kagome::runtime::binaryen

#endif  // KAGOME_CORE_RUNTIME_BINARYEN_MODULE_WASM_MODULE_FACTORY
