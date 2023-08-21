/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_WAVM_IMPL_MODULE_HPP
#define KAGOME_CORE_RUNTIME_WAVM_IMPL_MODULE_HPP

#include "runtime/module.hpp"

#include <memory>

#include "common/blob.hpp"
#include "log/logger.hpp"

namespace WAVM::Runtime {
  struct Compartment;
  struct Module;
  struct Object;
  using ImportBindings = std::vector<Object *>;
}  // namespace WAVM::Runtime

namespace kagome::runtime::wavm {

  class IntrinsicResolver;
  class InstanceEnvironmentFactory;
  class CompartmentWrapper;
  class IntrinsicModule;
  struct ModuleParams;

  class ModuleImpl final : public runtime::Module,
                           public std::enable_shared_from_this<ModuleImpl> {
   public:
    static std::shared_ptr<ModuleImpl> compileFrom(
        std::shared_ptr<CompartmentWrapper> compartment,
        ModuleParams &module_params,
        std::shared_ptr<IntrinsicModule> intrinsic_module,
        std::shared_ptr<const InstanceEnvironmentFactory> env_factory,
        gsl::span<const uint8_t> code,
        const common::Hash256 &code_hash);

    outcome::result<std::shared_ptr<ModuleInstance>> instantiate()
        const override;

    ModuleImpl(std::shared_ptr<CompartmentWrapper> compartment,
               std::shared_ptr<const IntrinsicModule> intrinsic_module,
               std::shared_ptr<const InstanceEnvironmentFactory> env_factory,
               std::shared_ptr<WAVM::Runtime::Module> module,
               const common::Hash256 &code_hash);

   private:
    WAVM::Runtime::ImportBindings link(IntrinsicResolver &resolver) const;

    std::shared_ptr<const InstanceEnvironmentFactory> env_factory_;
    std::shared_ptr<CompartmentWrapper> compartment_;
    std::shared_ptr<const IntrinsicModule> intrinsic_module_;
    std::shared_ptr<WAVM::Runtime::Module> module_;
    const common::Hash256 code_hash_;
    log::Logger logger_;

    friend class ModuleInstanceImpl;
  };

}  // namespace kagome::runtime::wavm

#endif  // KAGOME_CORE_RUNTIME_WAVM_IMPL_MODULE_HPP
