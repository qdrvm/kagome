/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wavm/impl/module_repository_impl.hpp"

#include <chrono>

#include <WAVM/Runtime/Runtime.h>

#include "intrinsic_functions.hpp"
#include "runtime/runtime_upgrade_tracker.hpp"
#include "runtime/wavm/intrinsic_resolver.hpp"

namespace kagome::runtime::wavm {

  ModuleRepositoryImpl::ModuleRepositoryImpl(
      std::shared_ptr<CompartmentWrapper> compartment,
      std::shared_ptr<const RuntimeUpgradeTracker> runtime_upgrade_tracker,
      std::shared_ptr<const crypto::Hasher> hasher,
      std::shared_ptr<IntrinsicResolver> resolver)
      : compartment_{compartment},
        runtime_upgrade_tracker_{std::move(runtime_upgrade_tracker)},
        resolver_{std::move(resolver)},
        hasher_{std::move(hasher)},
        logger_{log::createLogger(
            "ModuleRepositoryImpl", "runtime_api", soralog::Level::DEBUG)} {
    BOOST_ASSERT(compartment_);
    BOOST_ASSERT(resolver_);
    BOOST_ASSERT(runtime_upgrade_tracker_);
    BOOST_ASSERT(hasher_);
    BOOST_ASSERT(logger_);
  }

  outcome::result<std::shared_ptr<ModuleInstance>>
  ModuleRepositoryImpl::getInstanceAt(
      std::shared_ptr<const RuntimeCodeProvider> code_provider,
      const primitives::BlockInfo &block) {
    OUTCOME_TRY(state, runtime_upgrade_tracker_->getLastCodeUpdateState(block));
    OUTCOME_TRY(code, code_provider->getCodeAt(state));
    if (auto it = modules_.find(state); it == modules_.end()) {
      OUTCOME_TRY(module, loadFrom(code));
      modules_[state] = std::move(module);
    }
    if (auto it = instances_.find(state); it == instances_.end()) {
      instances_[state] = modules_[state]->instantiate(*resolver_);
    }
    return instances_[state];
  }

  outcome::result<std::unique_ptr<Module>> ModuleRepositoryImpl::loadFrom(
      gsl::span<const uint8_t> byte_code) {
    return Module::compileFrom(compartment_, byte_code);
  }

}  // namespace kagome::runtime::wavm
