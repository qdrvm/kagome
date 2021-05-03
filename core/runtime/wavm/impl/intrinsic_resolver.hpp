/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_WAVM_IMPL_INTRINSIC_RESOLVER_HPP
#define KAGOME_CORE_RUNTIME_WAVM_IMPL_INTRINSIC_RESOLVER_HPP

#include <WAVM/Runtime/Intrinsics.h>
#include <WAVM/Runtime/Linker.h>

#include "memory.hpp"

namespace WAVM::Runtime {
  struct Instance;
  struct Compartment;
  struct ContextRuntimeData;
}  // namespace WAVM::Runtime

namespace WAVM::Intrinsics {
  struct Module;
  struct Function;
}  // namespace WAVM::Intrinsics

namespace kagome::runtime::wavm {

  class IntrinsicResolver final : public WAVM::Runtime::Resolver {
   public:
    explicit IntrinsicResolver();
    ~IntrinsicResolver() override = default;

    bool resolve(const std::string &moduleName,
                 const std::string &exportName,
                 WAVM::IR::ExternType type,
                 WAVM::Runtime::Object *&outObject) override;

    //template <typename R, typename... Args>
    void addIntrinsic(
        std::string_view name,
        WAVM::Intrinsics::Function* func) {
      // side-effect of Function constructor is that this function is registered
      // in the module by pointer

     /* R (*dummy_pointer)(WAVM::Runtime::ContextRuntimeData *, Args...) {};
      auto type = WAVM::Intrinsics::inferIntrinsicFunctionType(dummy_pointer);
      auto function = std::make_unique<WAVM::Intrinsics::Function>(
          module_.get(), name.data(), static_cast<void *>(&func), type);*/
      functions_.emplace(name, func);
    }

    std::shared_ptr<Memory> getMemory() const {
      return memory_;
    }

   private:
    std::unique_ptr<WAVM::Intrinsics::Module> module_;
    // TODO(Harrm) cleanup
    WAVM::Runtime::Instance *module_instance_;
    std::shared_ptr<Memory> memory_;
    std::unordered_map<std::string_view,
                       WAVM::Intrinsics::Function*>
        functions_;
    // TODO(Harrm) cleanup
    WAVM::Runtime::Compartment *compartment_;
  };

}  // namespace kagome::runtime::wavm

#endif  // KAGOME_CORE_RUNTIME_WAVM_IMPL_INTRINSIC_RESOLVER_HPP
