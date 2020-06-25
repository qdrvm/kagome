/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_BINARYEN_WASM_MODULE_FACTORY
#define KAGOME_CORE_RUNTIME_BINARYEN_WASM_MODULE_FACTORY

namespace kagome::runtime::binaryen {

  class WasmModuleFactory {
   public:
    virtual ~WasmModuleFactory() = default;


  };

}

#endif  // KAGOME_CORE_RUNTIME_BINARYEN_WASM_MODULE_FACTORY
