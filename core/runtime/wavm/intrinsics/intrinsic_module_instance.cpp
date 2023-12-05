/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wavm/intrinsics/intrinsic_module_instance.hpp"

#include <WAVM/Runtime/Intrinsics.h>

#include "runtime/wavm/compartment_wrapper.hpp"
#include "runtime/wavm/intrinsics/intrinsic_functions.hpp"
#include "runtime/wavm/intrinsics/intrinsic_module.hpp"

namespace kagome::runtime::wavm {

  IntrinsicModuleInstance::IntrinsicModuleInstance(
      WAVM::Runtime::GCPointer<WAVM::Runtime::Instance> module_instance,
      std::shared_ptr<const CompartmentWrapper> compartment,
      WAVM::IR::MemoryType intrinsic_memory_type)
      : module_instance_{std::move(module_instance)},
        compartment_{std::move(compartment)},
        intrinsic_memory_type_{intrinsic_memory_type} {
    BOOST_ASSERT(compartment_);
    BOOST_ASSERT(module_instance_);
  }

  WAVM::Runtime::Memory *IntrinsicModuleInstance::getExportedMemory() const {
    return getTypedInstanceExport(module_instance_,
                                  IntrinsicModule::kIntrinsicMemoryName.data(),
                                  intrinsic_memory_type_);
  }

  WAVM::Runtime::Function *IntrinsicModuleInstance::getExportedFunction(
      const std::string &name, const WAVM::IR::FunctionType &type) const {
    // discard 'intrinsic' calling convention
    WAVM::IR::FunctionType wasm_type{type.results(), type.params()};
    return getTypedInstanceExport(module_instance_, name.data(), wasm_type);
  }

}  // namespace kagome::runtime::wavm
