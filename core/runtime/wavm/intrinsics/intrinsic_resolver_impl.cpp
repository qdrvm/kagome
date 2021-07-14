/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "intrinsic_resolver_impl.hpp"

#include <WAVM/Runtime/Intrinsics.h>

#include "intrinsic_functions.hpp"
#include "intrinsic_module_instance.hpp"
#include "runtime/wavm/compartment_wrapper.hpp"

namespace kagome::runtime::wavm {

  IntrinsicResolverImpl::IntrinsicResolverImpl(
      std::shared_ptr<IntrinsicModuleInstance> module_instance,
      std::shared_ptr<CompartmentWrapper> compartment)
      : module_instance_{std::move(module_instance)},
        compartment_{compartment} {
    BOOST_ASSERT(module_instance_ != nullptr);
    BOOST_ASSERT(compartment_ != nullptr);
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
      BOOST_ASSERT(export_func != nullptr);
      outObject = WAVM::Runtime::asObject(export_func);
      return true;
    }
    return false;
  }

  IntrinsicResolverImpl::~IntrinsicResolverImpl() {
    WAVM::Runtime::collectCompartmentGarbage(compartment_->getCompartment());
  }

}  // namespace kagome::runtime::wavm
