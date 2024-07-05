/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

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
        std::shared_ptr<CompartmentWrapper> compartment,
        std::shared_ptr<IntrinsicModuleInstance> module_instance);

    bool resolve(const std::string &moduleName,
                 const std::string &exportName,
                 WAVM::IR::ExternType type,
                 WAVM::Runtime::Object *&outObject) override;

   public:
    std::shared_ptr<CompartmentWrapper> compartment_;
    std::shared_ptr<IntrinsicModuleInstance> module_instance_;
    log::Logger logger_;
  };

}  // namespace kagome::runtime::wavm
