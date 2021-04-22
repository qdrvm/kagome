/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_WAVM_IMPL_MODULE_INSTANCE_HPP
#define KAGOME_CORE_RUNTIME_WAVM_IMPL_MODULE_INSTANCE_HPP

#include "runtime/wasm_result.hpp"
#include <string_view>

namespace WAVM::Runtime {
  class Instance;
  class Compartment;
}

namespace kagome::runtime::wavm {

  class ModuleInstance {
   public:
    explicit ModuleInstance(WAVM::Runtime::Instance *instance,
                            WAVM::Runtime::Compartment *compartment);

    WasmResult callExportFunction(std::string_view name, WasmResult args);

   private:
    WAVM::Runtime::Instance *instance_;

    WAVM::Runtime::Compartment *compartment_;
  };

}  // namespace kagome::runtime::wavm

#endif  // KAGOME_CORE_RUNTIME_WAVM_IMPL_MODULE_INSTANCE_HPP
