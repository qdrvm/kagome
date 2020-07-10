/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_BINARYEN_WASM_MODULE
#define KAGOME_CORE_RUNTIME_BINARYEN_WASM_MODULE

#include <binaryen/literal.h>
#include <binaryen/support/name.h>

namespace kagome::runtime::binaryen {

  /**
   * A wrapper for binaryen's wasm::Module and wasm::ModuleInstance
   */
  class WasmModule {
   public:
    virtual ~WasmModule() = default;

    /**
     * @param name the name of a wasm function to call
     * @param arguments the list of arguments to pass to the function
     * @return whatever the export function returns
     */
    virtual wasm::Literal callExport(
        wasm::Name name,
        const std::vector<wasm::Literal> &arguments) = 0;
  };

}  // namespace kagome::runtime::binaryen

#endif  // KAGOME_CORE_RUNTIME_BINARYEN_WASM_MODULE
