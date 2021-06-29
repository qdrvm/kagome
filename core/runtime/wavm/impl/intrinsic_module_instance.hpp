/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_WAVM_IMPL_INTRINSIC_MODULE_INSTANCE_HPP
#define KAGOME_CORE_RUNTIME_WAVM_IMPL_INTRINSIC_MODULE_INSTANCE_HPP

#include <memory>

#include <WAVM/IR/Types.h>
#include <WAVM/Runtime/Runtime.h>

namespace WAVM::Runtime {
  struct Function;

}  // namespace WAVM::Runtime

namespace kagome::runtime::wavm {

  class CompartmentWrapper;

  class IntrinsicModuleInstance final {
   public:
    IntrinsicModuleInstance(std::shared_ptr<CompartmentWrapper> compartment);

    WAVM::Runtime::Memory *getExportedMemory() const;
    WAVM::Runtime::Function *getExportedFunction(
        const std::string &name, WAVM::IR::FunctionType const &type) const;

    std::unique_ptr<IntrinsicModuleInstance> clone(
        std::shared_ptr<CompartmentWrapper> compartment) const;

   private:
    WAVM::Runtime::GCPointer<WAVM::Runtime::Instance> module_instance_;
  };

}  // namespace kagome::runtime::wavm

#endif  // KAGOME_CORE_RUNTIME_WAVM_IMPL_INTRINSIC_MODULE_INSTANCE_HPP
