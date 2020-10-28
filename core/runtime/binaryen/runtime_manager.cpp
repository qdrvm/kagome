/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/runtime_manager.hpp"

#include <gsl/gsl>

#include "crypto/hasher/hasher_impl.hpp"
#include "runtime/binaryen/runtime_external_interface.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::runtime::binaryen,
                            RuntimeManager::Error,
                            e) {
  using Error = kagome::runtime::binaryen::RuntimeManager::Error;
  switch (e) {
    case Error::EMPTY_STATE_CODE:
      return "Provided state code is empty, calling a function is impossible";
  }
  return "Unknown RuntimeManager error";
}

namespace kagome::runtime::binaryen {

  thread_local std::shared_ptr<RuntimeExternalInterface>
      RuntimeManager::external_interface_{};

  RuntimeManager::RuntimeManager(
      std::shared_ptr<extensions::ExtensionFactory> extension_factory,
      std::shared_ptr<WasmModuleFactory> module_factory,
      std::shared_ptr<TrieStorageProvider> storage_provider,
      std::shared_ptr<crypto::Hasher> hasher)
      : storage_provider_{std::move(storage_provider)},
        extension_factory_{std::move(extension_factory)},
        module_factory_{std::move(module_factory)},
        hasher_{std::move(hasher)} {
    BOOST_ASSERT(storage_provider_);
    BOOST_ASSERT(extension_factory_);
    BOOST_ASSERT(module_factory_);
    BOOST_ASSERT(hasher_);
  }

  outcome::result<RuntimeEnvironment>
  RuntimeManager::createPersistentRuntimeEnvironmentAt(
      const common::Buffer &state_code, const common::Hash256 &state_root) {
    OUTCOME_TRY(storage_provider_->setToPersistentAt(state_root));
    auto env = createRuntimeEnvironment(state_code);
    if (env.has_value()) {
      env.value().batch =
          storage_provider_->tryGetPersistentBatch().value()->batchOnTop();
    }
    return env;
  }

  outcome::result<RuntimeEnvironment>
  RuntimeManager::createEphemeralRuntimeEnvironmentAt(
      const common::Buffer &state_code, const common::Hash256 &state_root) {
    OUTCOME_TRY(storage_provider_->setToEphemeralAt(state_root));
    return createRuntimeEnvironment(state_code);
  }

  outcome::result<RuntimeEnvironment>
  RuntimeManager::createPersistentRuntimeEnvironment(
      const common::Buffer &state_code) {
    OUTCOME_TRY(storage_provider_->setToPersistent());
    auto env = createRuntimeEnvironment(state_code);
    if (env.has_value()) {
      env.value().batch =
          storage_provider_->tryGetPersistentBatch().value()->batchOnTop();
    }
    return env;
  }

  outcome::result<RuntimeEnvironment>
  RuntimeManager::createEphemeralRuntimeEnvironment(
      const common::Buffer &state_code) {
    OUTCOME_TRY(storage_provider_->setToEphemeral());
    return createRuntimeEnvironment(state_code);
  }

  outcome::result<RuntimeEnvironment> RuntimeManager::createRuntimeEnvironment(
      const common::Buffer &state_code) {
    if (state_code.empty()) {
      return Error::EMPTY_STATE_CODE;
    }

    auto hash = hasher_->twox_256(state_code);

    std::shared_ptr<WasmModule> module;

    // Trying retrieve pre-prepared module
    {
      std::lock_guard lockGuard(modules_mutex_);
      auto it = modules_.find(hash);
      if (it != modules_.end()) {
        module = it->second;
      }
    }

    if (external_interface_ == nullptr) {
      external_interface_ = std::make_shared<RuntimeExternalInterface>(
          extension_factory_, storage_provider_);
    }

    if (!module) {
      // Prepare new module
      OUTCOME_TRY(
          new_module,
          module_factory_->createModule(state_code, external_interface_));

      // Trying to safe emplace new module, and use existed one
      //  if it already emplaced in another thread
      std::lock_guard lockGuard(modules_mutex_);
      module = modules_.emplace(hash, std::move(new_module)).first->second;
    }

    return RuntimeEnvironment::create(
        external_interface_, module, state_code);
  }
  void RuntimeManager::reset() {
    external_interface_->reset();
  }

}  // namespace kagome::runtime::binaryen
