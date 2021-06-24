/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_BINARYEN_MODULE_WASM_MODULE_FACTORY_IMPL
#define KAGOME_CORE_RUNTIME_BINARYEN_MODULE_WASM_MODULE_FACTORY_IMPL

#include "runtime/binaryen/module/wasm_module_factory.hpp"

namespace kagome::runtime::binaryen {

  class WasmModuleFactoryImpl final : public WasmModuleFactory {
   public:
    ~WasmModuleFactoryImpl() override = default;

    outcome::result<std::unique_ptr<WasmModule>> createModule(
        const common::Buffer &code,
        std::shared_ptr<RuntimeExternalInterface> rei,
        std::shared_ptr<TrieStorageProvider> storage_provider) const override;
  };

}  // namespace kagome::runtime::binaryen

#endif  // KAGOME_CORE_RUNTIME_BINARYEN_MODULE_WASM_MODULE_FACTORY_IMPL
