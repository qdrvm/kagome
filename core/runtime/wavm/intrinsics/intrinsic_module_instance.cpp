/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "intrinsic_module_instance.hpp"

#include <WAVM/Runtime/Intrinsics.h>

#include "intrinsic_functions.hpp"
#include "intrinsic_module.hpp"
#include "runtime/wavm/compartment_wrapper.hpp"

namespace kagome::runtime::wavm {

  IntrinsicModuleInstance::IntrinsicModuleInstance(
      WAVM::Runtime::GCPointer<WAVM::Runtime::Instance> module_instance)
      : module_instance_{std::move(module_instance)} {}

  WAVM::Runtime::Memory *IntrinsicModuleInstance::getExportedMemory() const {
    return getTypedInstanceExport(module_instance_,
                                  IntrinsicModule::kIntrinsicMemoryName.data(),
                                  IntrinsicModule::kIntrinsicMemoryType);
  }

  WAVM::Runtime::Function *IntrinsicModuleInstance::getExportedFunction(
      const std::string &name, WAVM::IR::FunctionType const &type) const {
    // discard 'intrinsic' calling convention
    WAVM::IR::FunctionType wasm_type{type.results(), type.params()};
    return getTypedInstanceExport(module_instance_, name.data(), wasm_type);
  }

}  // namespace kagome::runtime::wavm
