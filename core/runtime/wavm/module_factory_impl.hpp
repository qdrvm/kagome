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
  class IntrinsicResolver;

  class ModuleFactoryImpl final : public ModuleFactory {
   public:
    enum class Error {
      RESOLVER_EXPIRED = 1
    };

    ModuleFactoryImpl(std::shared_ptr<CompartmentWrapper> compartment,
                      std::weak_ptr<IntrinsicResolver> resolver);

    outcome::result<std::unique_ptr<Module>> make(
        gsl::span<const uint8_t> code) const override;

   private:
    std::shared_ptr<CompartmentWrapper> compartment_;
    std::weak_ptr<IntrinsicResolver> resolver_;
  };

}  // namespace kagome::runtime::wavm

OUTCOME_HPP_DECLARE_ERROR(kagome::runtime::wavm, ModuleFactoryImpl::Error);

#endif  // KAGOME_CORE_RUNTIME_WAVM_MODULE_FACTORY_IMPL_HPP
