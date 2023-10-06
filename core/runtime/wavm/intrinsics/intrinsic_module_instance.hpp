/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_WAVM_IMPL_INTRINSIC_MODULE_INSTANCE_HPP
#define KAGOME_CORE_RUNTIME_WAVM_IMPL_INTRINSIC_MODULE_INSTANCE_HPP

#include <memory>

#include <WAVM/IR/Types.h>
#include <WAVM/Runtime/Intrinsics.h>
#include <WAVM/Runtime/Runtime.h>

namespace WAVM::Runtime {
  struct Function;

}  // namespace WAVM::Runtime

namespace kagome::runtime::wavm {

  class CompartmentWrapper;
  struct ModuleParams;

  /**
   * A wrapper around WAVM's intrinsic module instance
   * Exposes the host memory and Host API functions
   */
  class IntrinsicModuleInstance final {
   public:
    IntrinsicModuleInstance(
        WAVM::Runtime::GCPointer<WAVM::Runtime::Instance> module_instance,
        std::shared_ptr<const CompartmentWrapper> compartment,
        WAVM::IR::MemoryType intrinsic_memory_type);

    ~IntrinsicModuleInstance() {
      // to free the instance before compartment and thus
      // allow the compartment to clean up everything on its destruction
      module_instance_ = nullptr;
    }

    WAVM::Runtime::Memory *getExportedMemory() const;
    WAVM::Runtime::Function *getExportedFunction(
        const std::string &name, WAVM const ::IR::FunctionType &type) const;

   private:
    WAVM::Runtime::GCPointer<WAVM::Runtime::Instance> module_instance_;
    const std::shared_ptr<const CompartmentWrapper> compartment_;
    WAVM::IR::MemoryType intrinsic_memory_type_;
  };

}  // namespace kagome::runtime::wavm

#endif  // KAGOME_CORE_RUNTIME_WAVM_IMPL_INTRINSIC_MODULE_INSTANCE_HPP
