/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wavm/module_repository.hpp"

#include <WAVM/IR/FeatureSpec.h>
#include <WAVM/Runtime/Runtime.h>
#include <WAVM/WASM/WASM.h>

namespace kagome::runtime::wavm {

  ModuleRepository::ModuleRepository(
      std::shared_ptr<IntrinsicResolver> resolver,
      std::shared_ptr<RuntimeCodeProvider> code_provider)
      : compartment_{WAVM::Runtime::createCompartment("Runtime State")},
        code_provider_{std::move(code_provider)},
        resolver_{std::move(resolver)} {
    BOOST_ASSERT(compartment_);
    BOOST_ASSERT(resolver_);
    BOOST_ASSERT(code_provider_);
  }

  outcome::result<std::shared_ptr<ModuleInstance>>
  ModuleRepository::getInstanceAt(const storage::trie::RootHash &state) {
    if (auto it = modules_.find(state); it == modules_.end()) {
      OUTCOME_TRY(code, code_provider_->getCodeAt(state));
      OUTCOME_TRY(module, loadFrom(code));
      modules_[state] = std::move(module);
    }
    if (auto it = instances_.find(state); it == instances_.end()) {
      instances_[state] = modules_[state]->instantiate(*resolver_);
    }
    return instances_[state];
  }

  outcome::result<std::unique_ptr<Module>> ModuleRepository::loadFrom(
      gsl::span<const uint8_t> byte_code) {
    std::shared_ptr<WAVM::Runtime::Module> module = nullptr;
    WAVM::WASM::LoadError loadError;
    WAVM::IR::FeatureSpec featureSpec;

    if (!WAVM::Runtime::loadBinaryModule(byte_code.data(),
                                         byte_code.size(),
                                         module,
                                         featureSpec,
                                         &loadError)) {
      // TODO(Harrm): Introduce an error
      logger_->error("Error loading a WASM module: {}", loadError.message);
      return nullptr;
    }

    auto wasm_module =
        std::make_unique<Module>(compartment_, std::move(module));
    return wasm_module;
  }

}  // namespace kagome::runtime::wavm
