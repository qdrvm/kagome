/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/runtime_environment_factory_impl.hpp"

#include <gsl/span>

#include "crypto/hasher/hasher_impl.hpp"
#include "runtime/binaryen/binaryen_memory_provider.hpp"
#include "runtime/binaryen/runtime_external_interface.hpp"
#include "runtime/memory_provider.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::runtime::binaryen,
                            RuntimeEnvironmentFactoryImpl::Error,
                            e) {
  using Error = kagome::runtime::binaryen::RuntimeEnvironmentFactoryImpl::Error;
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
      RuntimeEnvironmentFactoryImpl::external_interface_{};
  thread_local std::shared_ptr<BinaryenMemoryProvider>
      RuntimeEnvironmentFactoryImpl::memory_provider_{};

  RuntimeEnvironmentFactoryImpl::RuntimeEnvironmentFactoryImpl(
      std::shared_ptr<CoreApiFactory> core_api_provider,
      std::shared_ptr<BinaryenWasmMemoryFactory> memory_factory,
      std::shared_ptr<host_api::HostApiFactory> host_api_factory,
      std::shared_ptr<WasmModuleFactory> module_factory,
      std::shared_ptr<RuntimeCodeProvider> wasm_provider,
      std::shared_ptr<TrieStorageProvider> storage_provider,
      std::shared_ptr<crypto::Hasher> hasher)
      : core_api_provider_{std::move(core_api_provider)},
        storage_provider_{std::move(storage_provider)},
        wasm_provider_{std::move(wasm_provider)},
        host_api_factory_{std::move(host_api_factory)},
        module_factory_{std::move(module_factory)},
        hasher_{std::move(hasher)},
        memory_factory_{std::move(memory_factory)} {
    BOOST_ASSERT(core_api_provider_);
    BOOST_ASSERT(memory_factory_);
    BOOST_ASSERT(wasm_provider_);
    BOOST_ASSERT(storage_provider_);
    BOOST_ASSERT(host_api_factory_);
    BOOST_ASSERT(module_factory_);
    BOOST_ASSERT(hasher_);
  }

  outcome::result<RuntimeEnvironment>
  RuntimeEnvironmentFactoryImpl::makeIsolated(const Config &config) {
    auto wasm_provider = config.wasm_provider.get_value_or(wasm_provider_);
    OUTCOME_TRY(code,
                wasm_provider->getCodeAt(storage_provider_->getLatestRoot()));
    return createIsolatedRuntimeEnvironment(common::Buffer{code});
  }

  outcome::result<RuntimeEnvironment>
  RuntimeEnvironmentFactoryImpl::makeIsolatedAt(
      const storage::trie::RootHash &state_root, const Config &config) {
    auto wasm_provider = config.wasm_provider.get_value_or(wasm_provider_);
    OUTCOME_TRY(code, wasm_provider->getCodeAt(state_root));
    return createIsolatedRuntimeEnvironment(common::Buffer{code});
  }

  outcome::result<RuntimeEnvironment>
  RuntimeEnvironmentFactoryImpl::makePersistentAt(
      const storage::trie::RootHash &state_root) {
    OUTCOME_TRY(storage_provider_->setToPersistentAt(state_root));

    auto persistent_batch = storage_provider_->tryGetPersistentBatch();
    if (!persistent_batch) return Error::NO_PERSISTENT_BATCH;

    OUTCOME_TRY(code, wasm_provider_->getCodeAt(state_root));
    auto env = createRuntimeEnvironment(common::Buffer{code});
    if (env.has_value()) {
      env.value().batch = (*persistent_batch)->batchOnTop();
    }

    return env;
  }

  outcome::result<RuntimeEnvironment>
  RuntimeEnvironmentFactoryImpl::makeEphemeralAt(
      const storage::trie::RootHash &state_root) {
    OUTCOME_TRY(storage_provider_->setToEphemeralAt(state_root));
    OUTCOME_TRY(code, wasm_provider_->getCodeAt(state_root));
    return createRuntimeEnvironment(common::Buffer{code});
  }

  outcome::result<RuntimeEnvironment>
  RuntimeEnvironmentFactoryImpl::makePersistent() {
    OUTCOME_TRY(storage_provider_->setToPersistent());

    auto persistent_batch = storage_provider_->tryGetPersistentBatch();
    if (!persistent_batch) return Error::NO_PERSISTENT_BATCH;

    OUTCOME_TRY(code,
                wasm_provider_->getCodeAt(storage_provider_->getLatestRoot()));
    auto env = createRuntimeEnvironment(common::Buffer{code});

    if (env.has_value()) {
      env.value().batch = (*persistent_batch)->batchOnTop();
    }

    return env;
  }

  outcome::result<RuntimeEnvironment>
  RuntimeEnvironmentFactoryImpl::makeEphemeral() {
    OUTCOME_TRY(storage_provider_->setToEphemeral());
    OUTCOME_TRY(code,
                wasm_provider_->getCodeAt(storage_provider_->getLatestRoot()));
    return createRuntimeEnvironment(common::Buffer{code});
  }

  outcome::result<RuntimeEnvironment>
  RuntimeEnvironmentFactoryImpl::createRuntimeEnvironment(
      const common::Buffer &state_code) {
    if (state_code.empty()) {
      return Error::EMPTY_STATE_CODE;
    }

    // TODO(Harrm): Refactor to runtime upgrade tracker as in WAVM
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

    if (memory_provider_ == nullptr) {
      memory_provider_ =
          std::make_shared<BinaryenMemoryProvider>(memory_factory_);
    }
    if (external_interface_ == nullptr) {
      external_interface_ =
          std::make_shared<RuntimeExternalInterface>(host_api_factory_->make(
              core_api_provider_, memory_provider_, storage_provider_));
      memory_provider_->setExternalInterface(external_interface_);
    }

    if (!module) {
      // Prepare new module
      OUTCOME_TRY(new_module,
                  module_factory_->createModule(
                      state_code, external_interface_, storage_provider_));

      // Trying to safe emplace new module, and use existed one
      //  if it already emplaced in another thread
      std::lock_guard lockGuard(modules_mutex_);
      module = modules_.emplace(hash, std::move(new_module)).first->second;
    }

    return RuntimeEnvironment::create(
        std::static_pointer_cast<MemoryProvider>(memory_provider_),
        external_interface_,
        module);
  }

  outcome::result<RuntimeEnvironment>
  RuntimeEnvironmentFactoryImpl::createIsolatedRuntimeEnvironment(
      const common::Buffer &state_code) {
    // TODO(Harrm): for review; doubt, maybe need a separate storage provider
    auto memory_provider =
        std::make_shared<BinaryenMemoryProvider>(memory_factory_);
    auto external_interface =
        std::make_shared<RuntimeExternalInterface>(host_api_factory_->make(
            core_api_provider_, memory_provider, storage_provider_));

    OUTCOME_TRY(module,
                module_factory_->createModule(
                    state_code, external_interface, storage_provider_));

    return RuntimeEnvironment::create(
        std::static_pointer_cast<MemoryProvider>(memory_provider_),
        external_interface,
        std::move(module));
  }

}  // namespace kagome::runtime::binaryen
