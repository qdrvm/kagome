/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "intrinsic_resolver_impl.hpp"

#include <WAVM/Runtime/Intrinsics.h>

#include "intrinsic_functions.hpp"
#include "intrinsic_module_instance.hpp"
#include "runtime/wavm/compartment_wrapper.hpp"

namespace kagome::runtime::wavm {

  IntrinsicResolverImpl::IntrinsicResolverImpl(
      std::shared_ptr<IntrinsicModuleInstance> module_instance)
      : module_instance_{std::move(module_instance)},
        logger_{log::createLogger("IntrinsicResolver", "wavm")} {
    BOOST_ASSERT(module_instance_ != nullptr);
  }

  bool IntrinsicResolverImpl::resolve(const std::string &moduleName,
                                      const std::string &exportName,
                                      WAVM::IR::ExternType type,
                                      WAVM::Runtime::Object *&outObject) {
    BOOST_ASSERT(moduleName == "env");
    if (exportName == "memory") {
      if (type.kind == WAVM::IR::ExternKind::memory) {
        outObject =
            WAVM::Runtime::asObject(module_instance_->getExportedMemory());
        return true;
      }
      return false;
    }
    if (type.kind == WAVM::IR::ExternKind::function) {
      auto export_func = module_instance_->getExportedFunction(
          exportName, asFunctionType(type));
      if (export_func == nullptr) {
        logger_->warn("Host function not implemented: {}", exportName);
        return false;
      }
      outObject = WAVM::Runtime::asObject(export_func);
      return true;
    }
    return false;
  }

}  // namespace kagome::runtime::wavm
