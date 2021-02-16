/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/runtime_environment_factory.hpp"

#include <gsl/gsl>

#include "crypto/hasher/hasher_impl.hpp"
#include "runtime/binaryen/runtime_external_interface.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::runtime::binaryen,
                            RuntimeEnvironmentFactory::Error,
                            e) {
  using Error = kagome::runtime::binaryen::RuntimeEnvironmentFactory::Error;
  switch (e) {
    case Error::EMPTY_STATE_CODE:
      return "Provided state code is empty, calling a function is impossible";
    case Error::NO_PERSISTENT_BATCH:
      return "No persistent batch in storage provider";
  }
  return "Unknown RuntimeEnvironmentFactory error";
}

namespace kagome::runtime::binaryen {

  thread_local std::shared_ptr<RuntimeExternalInterface>
      RuntimeEnvironmentFactory::external_interface_{};

  RuntimeEnvironmentFactory::RuntimeEnvironmentFactory(
      std::shared_ptr<BinaryenWasmMemoryFactory> memory_factory,
      std::shared_ptr<host_api::HostApiFactory> host_api_factory,
      std::shared_ptr<WasmModuleFactory> module_factory,
      std::shared_ptr<WasmProvider> wasm_provider,
      std::shared_ptr<TrieStorageProvider> storage_provider,
      std::shared_ptr<crypto::Hasher> hasher)
      : memory_factory_{std::move(memory_factory)},
        storage_provider_{std::move(storage_provider)},
        wasm_provider_{std::move(wasm_provider)},
        host_api_factory_{std::move(host_api_factory)},
        module_factory_{std::move(module_factory)},
        hasher_{std::move(hasher)} {
    BOOST_ASSERT(memory_factory_);
    BOOST_ASSERT(wasm_provider_);
    BOOST_ASSERT(storage_provider_);
    BOOST_ASSERT(host_api_factory_);
    BOOST_ASSERT(module_factory_);
    BOOST_ASSERT(hasher_);
  }

  outcome::result<RuntimeEnvironment>
  RuntimeEnvironmentFactory::makeIsolated() {
    return createIsolatedRuntimeEnvironment(
        wasm_provider_->getStateCodeAt(storage_provider_->getLatestRoot()));
  }

  outcome::result<RuntimeEnvironment> RuntimeEnvironmentFactory::makeIsolatedAt(
      const storage::trie::RootHash &state_root) {
    return createIsolatedRuntimeEnvironment(
        wasm_provider_->getStateCodeAt(state_root));
  }

  outcome::result<RuntimeEnvironment>
  RuntimeEnvironmentFactory::makePersistentAt(
      const storage::trie::RootHash &state_root) {
    OUTCOME_TRY(storage_provider_->setToPersistentAt(state_root));

    auto persistent_batch = storage_provider_->tryGetPersistentBatch();
    if (!persistent_batch) return Error::NO_PERSISTENT_BATCH;

    auto env = createRuntimeEnvironment(
        wasm_provider_->getStateCodeAt(storage_provider_->getLatestRoot()));
    if (env.has_value()) {
      env.value().batch = (*persistent_batch)->batchOnTop();
    }

    return env;
  }

  outcome::result<RuntimeEnvironment>
  RuntimeEnvironmentFactory::makeEphemeralAt(
      const storage::trie::RootHash &state_root) {
    OUTCOME_TRY(storage_provider_->setToEphemeralAt(state_root));
    return createRuntimeEnvironment(wasm_provider_->getStateCodeAt(state_root));
  }

  outcome::result<RuntimeEnvironment>
  RuntimeEnvironmentFactory::makePersistent() {
    OUTCOME_TRY(storage_provider_->setToPersistent());

    auto persistent_batch = storage_provider_->tryGetPersistentBatch();
    if (!persistent_batch) return Error::NO_PERSISTENT_BATCH;

    auto env = createRuntimeEnvironment(
        wasm_provider_->getStateCodeAt(storage_provider_->getLatestRoot()));
    if (env.has_value()) {
      env.value().batch = (*persistent_batch)->batchOnTop();
    }

    return env;
  }

  outcome::result<RuntimeEnvironment>
  RuntimeEnvironmentFactory::makeEphemeral() {
    OUTCOME_TRY(storage_provider_->setToEphemeral());
    return createRuntimeEnvironment(
        wasm_provider_->getStateCodeAt(storage_provider_->getLatestRoot()));
  }

  outcome::result<RuntimeEnvironment>
  RuntimeEnvironmentFactory::createRuntimeEnvironment(
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
          memory_factory_, host_api_factory_, storage_provider_);
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

    return RuntimeEnvironment::create(external_interface_, module);
  }

  outcome::result<RuntimeEnvironment>
  RuntimeEnvironmentFactory::createIsolatedRuntimeEnvironment(
      const common::Buffer &state_code) {
    // TODO(Harrm): for review; doubt, maybe need a separate storage provider
    auto external_interface = std::make_shared<RuntimeExternalInterface>(
        memory_factory_, host_api_factory_, storage_provider_);

    OUTCOME_TRY(module,
                module_factory_->createModule(state_code, external_interface));

    return RuntimeEnvironment::create(
        external_interface, std::move(module));
  }

}  // namespace kagome::runtime::binaryen
