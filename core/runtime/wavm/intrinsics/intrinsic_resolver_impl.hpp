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

#include "log/logger.hpp"

namespace WAVM::Runtime {
  struct Compartment;
  struct ContextRuntimeData;
}  // namespace WAVM::Runtime

namespace WAVM::Intrinsics {
  struct Function;
}  // namespace WAVM::Intrinsics

namespace kagome::runtime::wavm {
  class CompartmentWrapper;
  class MemoryImpl;
  class IntrinsicModuleInstance;

  class IntrinsicResolverImpl final : public IntrinsicResolver {
   public:
    IntrinsicResolverImpl(
        std::shared_ptr<IntrinsicModuleInstance> module_instance);

    bool resolve(const std::string &moduleName,
                 const std::string &exportName,
                 WAVM::IR::ExternType type,
                 WAVM::Runtime::Object *&outObject) override;

   private:
    std::shared_ptr<IntrinsicModuleInstance> module_instance_;
    log::Logger logger_;
  };

}  // namespace kagome::runtime::wavm

#endif  // KAGOME_CORE_RUNTIME_WAVM_IMPL_INTRINSIC_RESOLVER_IMPL_HPP
