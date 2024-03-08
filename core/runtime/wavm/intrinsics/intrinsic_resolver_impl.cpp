/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "intrinsic_resolver_impl.hpp"

#include <WAVM/IR/Operators.h>
#include <WAVM/Inline/Serialization.h>
#include <WAVM/Runtime/Linker.h>
#include <WAVM/Runtime/Runtime.h>

#include "intrinsic_module_instance.hpp"
#include "runtime/wavm/compartment_wrapper.hpp"

namespace kagome::runtime::wavm {

  IntrinsicResolverImpl::IntrinsicResolverImpl(
      std::shared_ptr<CompartmentWrapper> compartment,
      std::shared_ptr<IntrinsicModuleInstance> module_instance)
      : compartment_{std::move(compartment)},
        module_instance_{std::move(module_instance)},
        logger_{log::createLogger("IntrinsicResolver", "wavm")} {
    BOOST_ASSERT(compartment_ != nullptr);
    BOOST_ASSERT(module_instance_ != nullptr);
  }

  bool IntrinsicResolverImpl::resolve(const std::string &module_name,
                                      const std::string &export_name,
                                      WAVM::IR::ExternType type,
                                      WAVM::Runtime::Object *&outObject) {
    BOOST_ASSERT(module_name == "env");
    if (export_name == "memory") {
      if (type.kind == WAVM::IR::ExternKind::memory) {
        outObject =
            WAVM::Runtime::asObject(module_instance_->getExportedMemory());
        return true;
      }
      return false;
    }
    if (type.kind == WAVM::IR::ExternKind::function) {
      auto export_func = module_instance_->getExportedFunction(
          export_name, asFunctionType(type));
      if (export_func == nullptr) {
        WAVM::Serialization::ArrayOutputStream codeStream;
        WAVM::IR::OperatorEncoderStream encoder(codeStream);

        logger_->verbose("Generated stub for {}", export_name);
        WAVM::Runtime::generateStub("stubs",
                                    export_name,
                                    type,
                                    outObject,
                                    compartment_->getCompartment());
      } else {
        outObject = WAVM::Runtime::asObject(export_func);
      }
      return true;
    }
    return false;
  }

}  // namespace kagome::runtime::wavm
