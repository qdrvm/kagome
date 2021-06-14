/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wavm/impl/intrinsic_module_instance.hpp"

#include <WAVM/Runtime/Intrinsics.h>

#include "crutch.hpp"

namespace kagome::runtime::wavm {

  IntrinsicModuleInstance::IntrinsicModuleInstance(
      WAVM::Runtime::Compartment *compartment) {
    module_instance_ = WAVM::Intrinsics::instantiateModule(
        compartment, {getIntrinsicModule_env()}, "Host module");
  }

  WAVM::Runtime::Memory *IntrinsicModuleInstance::getExportedMemory() const {
    return getTypedInstanceExport(module_instance_,
                                  kIntrinsicMemoryName.data(),
                                  kIntrinsicMemoryType);
  }

  WAVM::Runtime::Function *IntrinsicModuleInstance::getExportedFunction(
      const std::string &name, WAVM::IR::FunctionType const &type) const {
    // discard 'intrinsic' calling convention
    WAVM::IR::FunctionType wasm_type{type.results(), type.params()};
    return getTypedInstanceExport(module_instance_, name.data(), wasm_type);
  }

  std::unique_ptr<IntrinsicModuleInstance> IntrinsicModuleInstance::clone(
      WAVM::Runtime::Compartment *compartment) const {
    return std::make_unique<IntrinsicModuleInstance>(compartment);
  }

}  // namespace kagome::runtime::wavm
