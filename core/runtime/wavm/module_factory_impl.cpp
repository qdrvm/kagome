/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wavm/module_factory_impl.hpp"

#include "runtime/wavm/module.hpp"
#include "runtime/wavm/module_cache.hpp"
#include "runtime/wavm/module_params.hpp"

namespace kagome::runtime::wavm {

  ModuleFactoryImpl::ModuleFactoryImpl(
      std::shared_ptr<CompartmentWrapper> compartment,
      std::shared_ptr<ModuleParams> module_params,
      std::shared_ptr<const InstanceEnvironmentFactory> env_factory,
      std::shared_ptr<IntrinsicModule> intrinsic_module,
      std::shared_ptr<ModuleCache> module_cache)
      : compartment_{std::move(compartment)},
        module_params_{std::move(module_params)},
        env_factory_{std::move(env_factory)},
        intrinsic_module_{std::move(intrinsic_module)} {
    BOOST_ASSERT(compartment_ != nullptr);
    BOOST_ASSERT(module_params_ != nullptr);
    BOOST_ASSERT(env_factory_ != nullptr);
    BOOST_ASSERT(intrinsic_module_ != nullptr);

    WAVM::Runtime::setGlobalObjectCache(std::move(module_cache));
  }

  outcome::result<std::unique_ptr<Module>> ModuleFactoryImpl::make(
      gsl::span<const uint8_t> code) const {
    return ModuleImpl::compileFrom(
        compartment_, *module_params_, intrinsic_module_, env_factory_, code);
  }

}  // namespace kagome::runtime::wavm
