/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_BINARYEN_MODULE_WASM_MODULE_FACTORY
#define KAGOME_CORE_RUNTIME_BINARYEN_MODULE_WASM_MODULE_FACTORY

#include "outcome/outcome.hpp"
#include "common/buffer.hpp"
#include "runtime/binaryen/runtime_external_interface.hpp"
#include "runtime/binaryen/module/wasm_module.hpp"

namespace kagome::runtime::binaryen {

  class WasmModuleFactory {
   public:
    virtual ~WasmModuleFactory() = default;

    virtual outcome::result<std::unique_ptr<WasmModule>> createModule(
        const common::Buffer &code,
        std::shared_ptr<RuntimeExternalInterface> rei) const = 0;
  };

}  // namespace kagome::runtime::binaryen

#endif  // KAGOME_CORE_RUNTIME_BINARYEN_MODULE_WASM_MODULE_FACTORY
