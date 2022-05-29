/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wavm/module.hpp"

#include <WAVM/Runtime/Linker.h>
#include <WAVM/Runtime/Runtime.h>
#include <WAVM/WASM/WASM.h>
#include <boost/assert.hpp>

#include "runtime/wavm/compartment_wrapper.hpp"
#include "runtime/wavm/instance_environment_factory.hpp"
#include "runtime/wavm/intrinsics/intrinsic_functions.hpp"
#include "runtime/wavm/intrinsics/intrinsic_module.hpp"
#include "runtime/wavm/intrinsics/intrinsic_resolver_impl.hpp"
#include "runtime/wavm/module_instance.hpp"
#include "runtime/wavm/module_params.hpp"

namespace kagome::runtime::wavm {

  std::unique_ptr<ModuleImpl> ModuleImpl::compileFrom(
      std::shared_ptr<CompartmentWrapper> compartment,
      std::shared_ptr<ModuleParams> module_params,
      std::shared_ptr<IntrinsicModule> intrinsic_module,
      std::shared_ptr<const InstanceEnvironmentFactory> env_factory,
      gsl::span<const uint8_t> code) {
    std::shared_ptr<WAVM::Runtime::Module> module = nullptr;
    WAVM::WASM::LoadError loadError;
    WAVM::IR::FeatureSpec featureSpec;

    log::Logger logger = log::createLogger("WAVM Module", "wavm");
    logger->info(
        "Compiling WebAssembly module for Runtime (going to take a few dozens "
        "of seconds)");
    if (!WAVM::Runtime::loadBinaryModule(
            code.data(), code.size(), module, featureSpec, &loadError)) {
      logger->critical("Error loading WAVM binary module: {}",
                       loadError.message);
      return nullptr;
    }

    auto imports = WAVM::Runtime::getModuleIR(module).memories.imports;
    if (imports.empty()) {
      logger->critical("Memory type not found in imports.");
      return nullptr;
    }

    module_params->intrinsicMemoryType = imports[0].type;
    intrinsic_module =
        std::make_shared<IntrinsicModule>(*intrinsic_module, module_params);
    runtime::wavm::registerHostApiMethods(*intrinsic_module);

    return std::unique_ptr<ModuleImpl>(
        new ModuleImpl{std::move(compartment),
                       std::move(intrinsic_module),
                       std::move(env_factory),
                       std::move(module)});
  }

  ModuleImpl::ModuleImpl(
      std::shared_ptr<CompartmentWrapper> compartment,
      std::shared_ptr<const IntrinsicModule> intrinsic_module,
      std::shared_ptr<const InstanceEnvironmentFactory> env_factory,
      std::shared_ptr<WAVM::Runtime::Module> module)
      : env_factory_{std::move(env_factory)},
        compartment_{std::move(compartment)},
        intrinsic_module_{std::move(intrinsic_module)},
        module_{std::move(module)},
        logger_{log::createLogger("WAVM Module", "wavm")} {
    BOOST_ASSERT(compartment_);
    BOOST_ASSERT(env_factory_);
    BOOST_ASSERT(intrinsic_module_);
    BOOST_ASSERT(module_);
  }

  outcome::result<std::shared_ptr<kagome::runtime::ModuleInstance>>
  ModuleImpl::instantiate() const {
    const auto &ir_module = WAVM::Runtime::getModuleIR(module_);
    bool imports_memory =
        std::find_if(ir_module.imports.cbegin(),
                     ir_module.imports.cend(),
                     [](auto &import) {
                       return import.kind == WAVM::IR::ExternKind::memory;
                     })
        != ir_module.imports.cend();
    auto memory_origin =
        imports_memory ? InstanceEnvironmentFactory::MemoryOrigin::EXTERNAL
                       : InstanceEnvironmentFactory::MemoryOrigin::INTERNAL;

    auto new_intrinsic_module_instance =
        std::shared_ptr<IntrinsicModuleInstance>(
            intrinsic_module_->instantiate());

    auto resolver =
        std::make_shared<IntrinsicResolverImpl>(new_intrinsic_module_instance);

    auto internal_instance =
        WAVM::Runtime::instantiateModule(compartment_->getCompartment(),
                                         module_,
                                         link(*resolver),
                                         "runtime_module");

    auto env = env_factory_->make(
        memory_origin, internal_instance, new_intrinsic_module_instance);

    auto instance = std::make_shared<ModuleInstance>(
        std::move(env), internal_instance, module_, compartment_);

    return instance;
  }

  WAVM::Runtime::ImportBindings ModuleImpl::link(
      IntrinsicResolver &resolver) const {
    auto &ir_module = WAVM::Runtime::getModuleIR(module_);

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
