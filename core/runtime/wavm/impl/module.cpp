/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "module.hpp"

#include <WAVM/Runtime/Linker.h>
#include <boost/assert.hpp>

#include "intrinsic_resolver.hpp"
#include "module_instance.hpp"
#include "gc_compartment.hpp"

namespace kagome::runtime::wavm {

  Module::Module(WAVM::Runtime::Compartment *compartment,
                 std::shared_ptr<WAVM::Runtime::Module> module)
      : compartment_{std::move(compartment)},
        module_{std::move(module)},
        logger_{log::createLogger("WAVM Module", "RuntimeAPI")} {
    BOOST_ASSERT(compartment_);
    BOOST_ASSERT(module_);
  }

  std::unique_ptr<ModuleInstance> Module::instantiate(
      IntrinsicResolver &resolver) {
    auto instance = WAVM::Runtime::instantiateModule(
        compartment_, module_, link(resolver), "test_module");
    return std::make_unique<ModuleInstance>(instance, compartment_);
  }

  WAVM::Runtime::ImportBindings Module::link(IntrinsicResolver &resolver) {
    auto ir_module = WAVM::Runtime::getModuleIR(module_);

    auto link_result = WAVM::Runtime::linkModule(ir_module, resolver);
    if (!link_result.success) {
      logger_->error("Failed to link module:");
      for (auto &import : link_result.missingImports) {
        logger_->error("\t{}::{}: {}",
                       import.moduleName,
                       import.exportName,
                       WAVM::IR::asString(import.type));
      }
      throw std::runtime_error("Failed to link module");
    }
    return link_result.resolvedImports;
  }

}  // namespace kagome::runtime::wavm
