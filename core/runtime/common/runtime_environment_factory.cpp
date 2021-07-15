/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/runtime_environment_factory.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::runtime,
                            RuntimeEnvironmentFactory::Error,
                            e) {
  using E = kagome::runtime::RuntimeEnvironmentFactory::Error;

  switch (e) {
    case E::PARENT_FACTORY_EXPIRED:
      return "The parent factory has expired";
  }
  return "Unknown runtime environment construction error";
}

namespace kagome::runtime {

  RuntimeEnvironment::RuntimeEnvironment(
      std::shared_ptr<const ModuleInstance> module_instance,
      Memory &memory,
      boost::optional<std::shared_ptr<storage::trie::PersistentTrieBatch>>
          batch,
      std::function<void(RuntimeEnvironment &)> on_destruction)
      : module_instance{std::move(module_instance)},
        memory{memory},
        batch{std::move(batch)},
        on_destruction_{std::move(on_destruction)} {}

  RuntimeEnvironment::~RuntimeEnvironment() {
    on_destruction_(*this);
  }

  RuntimeEnvironmentFactory::RuntimeEnvironmentTemplate::
      RuntimeEnvironmentTemplate(
          std::weak_ptr<RuntimeEnvironmentFactory> parent_factory)
      : state_{}, parent_factory_{std::move(parent_factory)} {
    BOOST_ASSERT(parent_factory_.lock() != nullptr);
    state_ = parent_factory_.lock()->latest_state_;
  }

  RuntimeEnvironmentFactory::RuntimeEnvironmentTemplate &
  RuntimeEnvironmentFactory::RuntimeEnvironmentTemplate::setState(
      const primitives::BlockInfo &state) {
    state_ = state;
    return *this;
  }

  RuntimeEnvironmentFactory::RuntimeEnvironmentTemplate &
  RuntimeEnvironmentFactory::RuntimeEnvironmentTemplate::persistent() {
    persistent_ = true;
    return *this;
  }

  outcome::result<RuntimeEnvironment>
  RuntimeEnvironmentFactory::RuntimeEnvironmentTemplate::make() {
    auto parent_factory = parent_factory_.lock();
    if (parent_factory == nullptr) {
      return RuntimeEnvironmentFactory::Error::PARENT_FACTORY_EXPIRED;
    }
    if (state_.hash == primitives::BlockHash{}) {
      OUTCOME_TRY(genesis_hash,
                  parent_factory->header_repo_->getHashByNumber(0));
      state_ = primitives::BlockInfo{0, genesis_hash};
    }
    OUTCOME_TRY(header,
                parent_factory->header_repo_->getBlockHeader(state_.hash));
    if (persistent_) {
      OUTCOME_TRY(parent_factory->storage_provider_->setToPersistentAt(
          header.state_root));
    } else {
      OUTCOME_TRY(parent_factory->storage_provider_->setToEphemeralAt(
          header.state_root));
    }

    OUTCOME_TRY(instance,
                parent_factory->module_repo_->getInstanceAt(
                    parent_factory->code_provider_, state_));
    OUTCOME_TRY(opt_heap_base, instance->getGlobal("__heap_base"));
    int32_t heap_base = boost::get<int32_t>(opt_heap_base.value());

    parent_factory->memory_provider_->resetMemory(heap_base);

    return RuntimeEnvironment{
        instance,
        parent_factory->memory_provider_->getCurrentMemory().value(),
        parent_factory->storage_provider_->tryGetPersistentBatch(),
        [weak_parent_factory = parent_factory_](auto &env) {
          auto parent_factory = weak_parent_factory.lock();
          if (parent_factory == nullptr) {
            return;
          }
          parent_factory->host_api_->reset();
          if (parent_factory->env_cleanup_callback_) {
            parent_factory->env_cleanup_callback_(env);
          }
        }};
  }

  RuntimeEnvironmentFactory::RuntimeEnvironmentFactory(
      std::shared_ptr<TrieStorageProvider> storage_provider,
      std::shared_ptr<host_api::HostApi> host_api,
      std::shared_ptr<MemoryProvider> memory_provider,
      std::shared_ptr<const runtime::RuntimeCodeProvider> code_provider,
      std::shared_ptr<ModuleRepository> module_repo,
      std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo)
      : latest_state_{},
        storage_provider_{std::move(storage_provider)},
        host_api_{std::move(host_api)},
        memory_provider_{std::move(memory_provider)},
        code_provider_{std::move(code_provider)},
        module_repo_{std::move(module_repo)},
        header_repo_{std::move(header_repo)},
        env_cleanup_callback_{} {
    BOOST_ASSERT(storage_provider_ != nullptr);
    BOOST_ASSERT(host_api_ != nullptr);
    BOOST_ASSERT(storage_provider_ != nullptr);
    BOOST_ASSERT(memory_provider_ != nullptr);
    BOOST_ASSERT(code_provider_ != nullptr);
    BOOST_ASSERT(module_repo_ != nullptr);
    BOOST_ASSERT(header_repo_ != nullptr);
  }

  std::unique_ptr<RuntimeEnvironmentFactory::RuntimeEnvironmentTemplate>
  RuntimeEnvironmentFactory::start() {
    return std::make_unique<RuntimeEnvironmentTemplate>(weak_from_this());
  }

  void RuntimeEnvironmentFactory::setEnvCleanupCallback(
      std::function<void(RuntimeEnvironment &)> &&callback) {
    env_cleanup_callback_ = std::move(callback);
  }

}  // namespace kagome::runtime
