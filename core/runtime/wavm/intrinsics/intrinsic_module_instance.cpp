/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wavm/intrinsics/intrinsic_module_instance.hpp"

#include <WAVM/Runtime/Intrinsics.h>

#include "runtime/wavm/compartment_wrapper.hpp"
#include "runtime/wavm/intrinsics/intrinsic_functions.hpp"
#include "runtime/wavm/intrinsics/intrinsic_module.hpp"

namespace kagome::runtime::wavm {

  size_t IntrinsicModuleInstance::Count = 0;

  IntrinsicModuleInstance::IntrinsicModuleInstance(
      WAVM::Runtime::GCPointer<WAVM::Runtime::Instance> module_instance,
      std::shared_ptr<const CompartmentWrapper> compartment)
      : module_instance_{std::move(module_instance)},
        compartment_{std::move(compartment)} {
    BOOST_ASSERT(compartment_);
    Count++;
  }

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
