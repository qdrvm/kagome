/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wavm/module_factory_impl.hpp"

#include "runtime/wavm/module.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::runtime::wavm,
                            ModuleFactoryImpl::Error,
                            e) {
  using E = kagome::runtime::wavm::ModuleFactoryImpl::Error;

  switch (e) {
    case E::RESOLVER_EXPIRED:
      return "Attempt to use an expired intrinsic resolver";
  }
  return "Unknown wavm's ModuleFactory error";
}

namespace kagome::runtime::wavm {

  ModuleFactoryImpl::ModuleFactoryImpl(
      std::shared_ptr<CompartmentWrapper> compartment,
      std::weak_ptr<IntrinsicResolver> resolver)
      : compartment_{std::move(compartment)}, resolver_{std::move(resolver)} {
    BOOST_ASSERT(compartment_ != nullptr);
    BOOST_ASSERT(resolver_.lock() != nullptr);
  }

  outcome::result<std::unique_ptr<Module>> ModuleFactoryImpl::make(
      gsl::span<const uint8_t> code) const {
    if (resolver_.lock() == nullptr) {
      return Error::RESOLVER_EXPIRED;
    }
    return ModuleImpl::compileFrom(compartment_, resolver_.lock(), code);
  }

}  // namespace kagome::runtime::wavm
