/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wavm/module_repository.hpp"

#include <chrono>

#include <WAVM/IR/FeatureSpec.h>
#include <WAVM/Runtime/Runtime.h>
#include <WAVM/WASM/WASM.h>

#include "crutch.hpp"
#include "gc_compartment.hpp"

namespace kagome::runtime::wavm {

  ModuleRepository::ModuleRepository(
      std::shared_ptr<crypto::Hasher> hasher,
      std::shared_ptr<Memory> memory,
      std::shared_ptr<IntrinsicResolver> resolver,
      std::shared_ptr<RuntimeCodeProvider> code_provider)
      : compartment_{getCompartment()},
        code_provider_{std::move(code_provider)},
        resolver_{std::move(resolver)},
        memory_{std::move(memory)},
        hasher_{std::move(hasher)},
        logger_{log::createLogger("ModuleRepository", "runtime_api", soralog::Level::DEBUG)} {
    BOOST_ASSERT(compartment_);
    BOOST_ASSERT(resolver_);
    BOOST_ASSERT(code_provider_);
    BOOST_ASSERT(memory_);
    BOOST_ASSERT(hasher_);
    BOOST_ASSERT(logger_);
  }

  outcome::result<std::shared_ptr<ModuleInstance>>
  ModuleRepository::getInstanceAt(const storage::trie::RootHash &state) {
    // TODO(Harrm): Code's one big blob, should pay attention to how it's stored
    // and where it can be needlessly copied/IOed to/from DB
    std::chrono::high_resolution_clock clock{};
    auto start = clock.now();
    OUTCOME_TRY(code, code_provider_->getCodeAt(state));
    auto code_hash = hasher_->sha2_256(code);
    auto end = clock.now();
    logger_->debug("Getting and hashing module code: {} us",
             std::chrono::duration_cast<std::chrono::microseconds>(end - start)
                 .count());

    if (auto it = modules_.find(code_hash); it == modules_.end()) {
      OUTCOME_TRY(module, loadFrom(code));
      modules_[code_hash] = std::move(module);
    }
    if (auto it = instances_.find(code_hash); it == instances_.end()) {
      start = clock.now();
      instances_[code_hash] = modules_[code_hash]->instantiate(*resolver_);
      auto heap_base = instances_[code_hash]->getGlobal("__heap_base");
      BOOST_ASSERT(heap_base.has_value()
                   && heap_base.value().type == ValueType::i32);
      memory_->setHeapBase(heap_base.value().i32);
      memory_->reset();
      end = clock.now();
      logger_->debug("Instantiation of a module: {} us",
                     std::chrono::duration_cast<std::chrono::microseconds>(end - start)
                         .count());
    }
    return instances_[code_hash];
  }

  outcome::result<std::unique_ptr<Module>> ModuleRepository::loadFrom(
      gsl::span<const uint8_t> byte_code) {
    // TODO(Harrm): Might want to cache here as well, e.g. for MiscExtension
    // calls
    std::shared_ptr<WAVM::Runtime::Module> module = nullptr;
    WAVM::WASM::LoadError loadError;
    WAVM::IR::FeatureSpec featureSpec;

    logger_->verbose(
        "Compiling WebAssembly module for Runtime (going to take a few dozens "
        "of seconds)");
    if (!WAVM::Runtime::loadBinaryModule(byte_code.data(),
                                         byte_code.size(),
                                         module,
                                         featureSpec,
                                         &loadError)) {
      // TODO(Harrm): Introduce an outcome error
      logger_->error("Error loading a WASM module: {}", loadError.message);
      return nullptr;
    }

    auto wasm_module =
        std::make_unique<Module>(compartment_, std::move(module));
    return wasm_module;
  }

}  // namespace kagome::runtime::wavm
