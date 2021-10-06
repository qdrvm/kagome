/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/runtime_environment_factory.hpp"

#include "log/profiling_logger.hpp"
#include "runtime/instance_environment.hpp"
#include "storage/trie/polkadot_trie/trie_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::runtime,
                            RuntimeEnvironmentFactory::Error,
                            e) {
  using E = kagome::runtime::RuntimeEnvironmentFactory::Error;

  switch (e) {
    case E::PARENT_FACTORY_EXPIRED:
      return "The parent factory has expired";
    case E::ABSENT_BLOCK:
      return "Failed to obtain the required block from storage";
    case E::ABSENT_HEAP_BASE:
      return "Failed to extract heap base from a module";
    case E::FAILED_TO_SET_STORAGE_STATE:
      return "Failed to set the storage state to the desired value";
  }
  return "Unknown runtime environment construction error";
}

namespace kagome::runtime {

  RuntimeEnvironment::RuntimeEnvironment(
      std::shared_ptr<ModuleInstance> module_instance,
      std::shared_ptr<const MemoryProvider> memory_provider,
      std::shared_ptr<const TrieStorageProvider> storage_provider)
      : module_instance{std::move(module_instance)},
        memory_provider{std::move(memory_provider)},
        storage_provider{std::move(storage_provider)} {
    BOOST_ASSERT(this->module_instance);
    BOOST_ASSERT(this->memory_provider);
    BOOST_ASSERT(this->storage_provider);
  }

  RuntimeEnvironmentFactory::RuntimeEnvironmentTemplate::
      RuntimeEnvironmentTemplate(
          std::weak_ptr<const RuntimeEnvironmentFactory> parent_factory,
          const primitives::BlockInfo &blockchain_state,
          const storage::trie::RootHash &storage_state)
      : blockchain_state_{blockchain_state},
        storage_state_{storage_state},
        parent_factory_{std::move(parent_factory)} {
    BOOST_ASSERT(parent_factory_.lock() != nullptr);
  }

  RuntimeEnvironmentFactory::RuntimeEnvironmentTemplate &
  RuntimeEnvironmentFactory::RuntimeEnvironmentTemplate::persistent() {
    persistent_ = true;
    return *this;
  }

  outcome::result<std::unique_ptr<RuntimeEnvironment>>
  RuntimeEnvironmentFactory::RuntimeEnvironmentTemplate::make() {
    KAGOME_PROFILE_START(runtime_env_making);
    auto parent_factory = parent_factory_.lock();
    if (parent_factory == nullptr) {
      return RuntimeEnvironmentFactory::Error::PARENT_FACTORY_EXPIRED;
    }
    auto header_res =
        parent_factory->header_repo_->getBlockHeader(blockchain_state_.hash);
    if (!header_res) {
      parent_factory->logger_->error(
          "Failed to obtain the block header with hash {} when initializing a "
          "runtime environment; Reason: {}",
          blockchain_state_.hash.toHex(),
          header_res.error().message());
      return Error::ABSENT_BLOCK;
    }

    OUTCOME_TRY(instance,
                parent_factory->module_repo_->getInstanceAt(
                    parent_factory->code_provider_,
                    blockchain_state_,
                    header_res.value()));

    auto &env = instance->getEnvironment();
    if (persistent_) {
      if (auto res = env.storage_provider->setToPersistentAt(storage_state_);
          !res) {
        parent_factory->logger_->error(
            "Failed to set the storage state to hash {} when initializing a "
            "runtime environment; Reason: {}",
            storage_state_.toHex(),
            res.error().message());
        return Error::FAILED_TO_SET_STORAGE_STATE;
      }
    } else {
      if (auto res = env.storage_provider->setToEphemeralAt(storage_state_);
          !res) {
        parent_factory->logger_->error(
            "Failed to set the storage state to hash {} when initializing a "
            "runtime environment; Reason: {}",
            storage_state_.toHex(),
            res.error().message());
        return Error::FAILED_TO_SET_STORAGE_STATE;
      }
    }

    auto opt_heap_base = instance->getGlobal("__heap_base");
    if (!opt_heap_base.has_value() || !opt_heap_base.value()) {
      parent_factory->logger_->error(
          "Failed to obtain __heap_base from a runtime module; Reason: ");
      return Error::ABSENT_HEAP_BASE;
    }
    int32_t heap_base = boost::get<int32_t>(opt_heap_base.value().value());

    OUTCOME_TRY(env.memory_provider->resetMemory(heap_base));

    OUTCOME_TRY(heappages_key, common::Buffer::fromString(":heappages"));
    auto heappages_res =
        env.storage_provider->getCurrentBatch()->get(heappages_key);
    if (heappages_res.has_value()) {
      auto &&heappages = heappages_res.value();
      if (sizeof(uint64_t) != heappages.size()) {
        parent_factory->logger_->error(
            "Unable to read :heappages value. Type size mismatch. "
            "Required {} bytes, but {} available",
            sizeof(uint64_t),
            heappages.size());
      } else {
        uint64_t pages = common::bytes_to_uint64_t(heappages.asVector());
        env.memory_provider->getCurrentMemory()->resize(pages
                                                        * kMemoryPageSize);
        parent_factory->logger_->trace(
            "Creating wasm module with non-default :heappages value set to {}",
            pages);
      }
    } else if (kagome::storage::trie::TrieError::NO_VALUE
               != heappages_res.error()) {
      return heappages_res.error();
    }

    SL_DEBUG(parent_factory->logger_,
             "Runtime environment at #{} hash: {}, state: {}",
             blockchain_state_.number,
             blockchain_state_.hash.toHex(),
             storage_state_.toHex());

    auto runtime_env = std::make_unique<RuntimeEnvironment>(
        instance, env.memory_provider, env.storage_provider);
    KAGOME_PROFILE_END(runtime_env_making);
    return runtime_env;
  }

  RuntimeEnvironmentFactory::RuntimeEnvironmentFactory(
      std::shared_ptr<const runtime::RuntimeCodeProvider> code_provider,
      std::shared_ptr<ModuleRepository> module_repo,
      std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo)
      : code_provider_{std::move(code_provider)},
        module_repo_{std::move(module_repo)},
        header_repo_{std::move(header_repo)},
        logger_{log::createLogger("RuntimeEnvironmentFactory", "runtime")} {
    BOOST_ASSERT(code_provider_ != nullptr);
    BOOST_ASSERT(module_repo_ != nullptr);
    BOOST_ASSERT(header_repo_ != nullptr);
  }

  std::unique_ptr<RuntimeEnvironmentFactory::RuntimeEnvironmentTemplate>
  RuntimeEnvironmentFactory::start(
      const primitives::BlockInfo &blockchain_state,
      const storage::trie::RootHash &storage_state) const {
    return std::make_unique<RuntimeEnvironmentTemplate>(
        weak_from_this(), blockchain_state, storage_state);
  }

  outcome::result<
      std::unique_ptr<RuntimeEnvironmentFactory::RuntimeEnvironmentTemplate>>
  RuntimeEnvironmentFactory::start(
      const primitives::BlockHash &block_hash) const {
    OUTCOME_TRY(header, header_repo_->getBlockHeader(block_hash));
    return start({header.number, std::move(block_hash)},
                 std::move(header.state_root));
  }

  outcome::result<
      std::unique_ptr<RuntimeEnvironmentFactory::RuntimeEnvironmentTemplate>>
  RuntimeEnvironmentFactory::start() const {
    auto genesis_hash = header_repo_->getHashByNumber(0);
    if (!genesis_hash) {
      logger_->error(
          "Failed to obtain the genesis block for runtime executor "
          "initialization; Reason: {}",
          genesis_hash.error().message());
      return Error::ABSENT_BLOCK;
    }
    return start(genesis_hash.value());
  }

}  // namespace kagome::runtime
