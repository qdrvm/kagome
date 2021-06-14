/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wavm/impl/module_repository_impl.hpp"

#include <chrono>

#include <WAVM/Runtime/Runtime.h>

#include "runtime/wavm/intrinsic_resolver.hpp"
#include "crutch.hpp"

namespace kagome::runtime::wavm {

  ModuleRepositoryImpl::ModuleRepositoryImpl(
      WAVM::Runtime::Compartment* compartment,
      std::shared_ptr<const crypto::Hasher> hasher,
      std::shared_ptr<IntrinsicResolver> resolver)
      : compartment_{compartment},
        resolver_{std::move(resolver)},
        hasher_{std::move(hasher)},
        logger_{log::createLogger(
            "ModuleRepositoryImpl", "runtime_api", soralog::Level::DEBUG)} {
    BOOST_ASSERT(compartment_);
    BOOST_ASSERT(resolver_);
    BOOST_ASSERT(hasher_);
    BOOST_ASSERT(logger_);
  }

  outcome::result<std::shared_ptr<ModuleInstance>>
  ModuleRepositoryImpl::getInstanceAt(
      std::shared_ptr<RuntimeCodeProvider> code_provider,
      const primitives::BlockInfo &block) {
    OUTCOME_TRY(code_and_state, code_provider->getCodeAt(block));
    return getInstanceAt_Internal(code_and_state.code,
                                  std::move(code_and_state.state));
  }

  outcome::result<std::shared_ptr<ModuleInstance>>
  ModuleRepositoryImpl::getInstanceAtLatest(
      std::shared_ptr<RuntimeCodeProvider> code_provider) {
    OUTCOME_TRY(code_and_state, code_provider->getLatestCode());
    return getInstanceAt_Internal(code_and_state.code,
                                  std::move(code_and_state.state));
  }

  outcome::result<std::shared_ptr<ModuleInstance>>
  ModuleRepositoryImpl::getInstanceAt_Internal(gsl::span<const uint8_t> code,
                                           storage::trie::RootHash state) {
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
    // TODO(Harrm): Might want to cache here as well, e.g. for MiscExtension
    // calls
    return Module::compileFrom(compartment_, byte_code);
  }

}  // namespace kagome::runtime::wavm
