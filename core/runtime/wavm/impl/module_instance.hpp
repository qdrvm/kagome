/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_WAVM_IMPL_MODULE_INSTANCE_HPP
#define KAGOME_CORE_RUNTIME_WAVM_IMPL_MODULE_INSTANCE_HPP

#include <string_view>

#include <boost/optional.hpp>

#include "runtime/wasm_result.hpp"

namespace WAVM{
  namespace Runtime {
    class Instance;
    class Compartment;
  }
  namespace IR {
    class Value;
  }
}

namespace kagome::runtime::wavm {

  class ModuleInstance {
   public:
    explicit ModuleInstance(WAVM::Runtime::Instance *instance,
                            WAVM::Runtime::Compartment *compartment);

    ~ModuleInstance() {

    }

    WasmResult callExportFunction(std::string_view name, WasmResult args);

    boost::optional<WAVM::IR::Value> getGlobal(std::string_view name);

   private:
    WAVM::Runtime::Instance *instance_;
    WAVM::Runtime::Compartment *compartment_;
  };

}  // namespace kagome::runtime::wavm

#endif  // KAGOME_CORE_RUNTIME_WAVM_IMPL_MODULE_INSTANCE_HPP
