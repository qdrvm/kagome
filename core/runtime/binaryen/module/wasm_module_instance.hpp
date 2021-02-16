/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_BINARYEN_MODULE_WASM_MODULE_INSTANCE
#define KAGOME_CORE_RUNTIME_BINARYEN_MODULE_WASM_MODULE_INSTANCE

#include <binaryen/literal.h>
#include <binaryen/support/name.h>
#include <boost/optional.hpp>

namespace kagome::runtime::binaryen {

  /**
   * Wrapper for wasm::ModuleInstance
   */
  class WasmModuleInstance {
   public:
    virtual ~WasmModuleInstance() = default;

    /**
     * @param name the name of a wasm function to call
     * @param arguments the list of arguments to pass to the function
     * @return whatever the export function returns
     */
    virtual wasm::Literal callExportFunction(
        wasm::Name name, const std::vector<wasm::Literal> &arguments) = 0;

    /**
     * @param name the name of a wasm global
     * @return whatever the export global returns
     */
    virtual wasm::Literal getExportGlobal(wasm::Name name) = 0;
  };
}  // namespace kagome::runtime::binaryen

#endif  // KAGOME_CORE_RUNTIME_BINARYEN_MODULE_WASM_MODULE_INSTANCE
