/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/module_factory.hpp"

#include "outcome/outcome.hpp"

namespace kagome::application {
  class AppConfiguration;
}
namespace kagome::crypto {
  class Hasher;
}

namespace kagome::runtime::wavm {

  class CompartmentWrapper;
  class InstanceEnvironmentFactory;
  class IntrinsicModule;
  struct ModuleParams;
  struct ModuleCache;

  class ModuleFactoryImpl final : public ModuleFactory {
   public:
    ModuleFactoryImpl(
        std::shared_ptr<CompartmentWrapper> compartment,
        std::shared_ptr<ModuleParams> module_params,
        std::shared_ptr<const InstanceEnvironmentFactory> env_factory,
        std::shared_ptr<IntrinsicModule> intrinsic_module,
        std::optional<std::shared_ptr<ModuleCache>> module_cache,
        std::shared_ptr<crypto::Hasher> hasher);

    outcome::result<std::shared_ptr<Module>, CompilationError> make(
        common::BufferView code) const override;

   private:
    std::shared_ptr<CompartmentWrapper> compartment_;
    std::shared_ptr<ModuleParams> module_params_;
    std::shared_ptr<const InstanceEnvironmentFactory> env_factory_;
    std::shared_ptr<IntrinsicModule> intrinsic_module_;
    std::shared_ptr<crypto::Hasher> hasher_;
  };

}  // namespace kagome::runtime::wavm
