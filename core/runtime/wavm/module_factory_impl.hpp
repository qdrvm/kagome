/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_WAVM_MODULE_FACTORY_IMPL_HPP
#define KAGOME_CORE_RUNTIME_WAVM_MODULE_FACTORY_IMPL_HPP

#include "runtime/module_factory.hpp"

#include "outcome/outcome.hpp"

namespace kagome::runtime::wavm {

  class CompartmentWrapper;
  class InstanceEnvironmentFactory;

  class ModuleFactoryImpl final : public ModuleFactory {
   public:
    ModuleFactoryImpl(
        std::shared_ptr<CompartmentWrapper> compartment,
        std::shared_ptr<const InstanceEnvironmentFactory> env_factory);

    outcome::result<std::unique_ptr<Module>> make(
        gsl::span<const uint8_t> code) const override;

   private:
    std::shared_ptr<CompartmentWrapper> compartment_;
    std::shared_ptr<const InstanceEnvironmentFactory> env_factory_;
  };

}  // namespace kagome::runtime::wavm

#endif  // KAGOME_CORE_RUNTIME_WAVM_MODULE_FACTORY_IMPL_HPP
