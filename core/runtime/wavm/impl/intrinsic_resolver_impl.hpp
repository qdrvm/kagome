/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_WAVM_IMPL_INTRINSIC_RESOLVER_IMPL_HPP
#define KAGOME_CORE_RUNTIME_WAVM_IMPL_INTRINSIC_RESOLVER_IMPL_HPP

#include "runtime/wavm/intrinsic_resolver.hpp"

#include <unordered_map>

#include <WAVM/Runtime/Intrinsics.h>
#include <WAVM/Runtime/Linker.h>

namespace WAVM::Runtime {
  struct Compartment;
  struct ContextRuntimeData;
}  // namespace WAVM::Runtime

namespace WAVM::Intrinsics {
  struct Function;
}  // namespace WAVM::Intrinsics

namespace kagome::runtime::wavm {
  class CompartmentWrapper;
  class Memory;
  class IntrinsicModuleInstance;

  class IntrinsicResolverImpl final : public IntrinsicResolver {
   public:
    IntrinsicResolverImpl(
        std::shared_ptr<IntrinsicModuleInstance> module_instance,
        std::shared_ptr<CompartmentWrapper> compartment);
    ~IntrinsicResolverImpl() override;

    bool resolve(const std::string &moduleName,
                 const std::string &exportName,
                 WAVM::IR::ExternType type,
                 WAVM::Runtime::Object *&outObject) override;

    void addIntrinsic(std::string_view name, WAVM::Intrinsics::Function *func) override {
      functions_.emplace(name, func);
    }

    std::unique_ptr<IntrinsicResolver> clone() const override;

   private:
    std::shared_ptr<IntrinsicModuleInstance> module_instance_;
    std::shared_ptr<CompartmentWrapper> compartment_;
    std::unordered_map<std::string_view, WAVM::Intrinsics::Function *>
        functions_;
  };

}  // namespace kagome::runtime::wavm

#endif  // KAGOME_CORE_RUNTIME_WAVM_IMPL_INTRINSIC_RESOLVER_IMPL_HPP
