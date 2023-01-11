/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/runtime_environment_factory.hpp"

#include "log/profiling_logger.hpp"
#include "runtime/common/uncompress_code_if_needed.hpp"
#include "runtime/instance_environment.hpp"
#include "runtime/module.hpp"
#include "runtime/module_factory.hpp"
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
  using namespace kagome::common::literals;

  RuntimeEnvironment::RuntimeEnvironment(
      std::shared_ptr<ModuleInstance> module_instance,
      std::shared_ptr<const MemoryProvider> memory_provider,
      std::shared_ptr<TrieStorageProvider> storage_provider,
      primitives::BlockInfo blockchain_state)
      : module_instance{std::move(module_instance)},
        memory_provider{std::move(memory_provider)},
        storage_provider{std::move(storage_provider)},
        blockchain_state_{std::move(blockchain_state)} {
    BOOST_ASSERT(this->module_instance);
    BOOST_ASSERT(this->memory_provider);
    BOOST_ASSERT(this->storage_provider);
  }

  outcome::result<RuntimeEnvironment> RuntimeEnvironment::fromCode(
      const runtime::ModuleFactory &module_factory,
      common::BufferView code_zstd) {
    common::Buffer code;
    OUTCOME_TRY(runtime::uncompressCodeIfNeeded(code_zstd, code));
    OUTCOME_TRY(module, module_factory.make(code));
    OUTCOME_TRY(instance, module->instantiate());
    runtime::RuntimeEnvironment env{
        instance,
        instance->getEnvironment().memory_provider,
        instance->getEnvironment().storage_provider,
        {},
    };
    env.storage_provider->setToEphemeralAt(storage::trie::kEmptyRootHash)
        .value();
    OUTCOME_TRY(resetMemory(*instance));
    return env;
  }

  outcome::result<void> resetMemory(const ModuleInstance &instance) {
    static auto log = log::createLogger("RuntimeEnvironmentFactory", "runtime");

    OUTCOME_TRY(opt_heap_base, instance.getGlobal("__heap_base"));
    if (not opt_heap_base) {
      log->error(
          "__heap_base global variable is not found in a runtime module");
      return RuntimeEnvironmentFactory::Error::ABSENT_HEAP_BASE;
    }
    int32_t heap_base = boost::get<int32_t>(*opt_heap_base);

    auto &memory_provider = instance.getEnvironment().memory_provider;
    OUTCOME_TRY(
        const_cast<MemoryProvider &>(*memory_provider).resetMemory(heap_base));
    auto &memory = memory_provider->getCurrentMemory()->get();

    static auto heappages_key = ":heappages"_buf;
    OUTCOME_TRY(
        heappages,
        instance.getEnvironment().storage_provider->getCurrentBatch()->tryGet(
            heappages_key));
    if (heappages) {
      if (sizeof(uint64_t) != heappages->size()) {
        log->error(
            "Unable to read :heappages value. Type size mismatch. "
            "Required {} bytes, but {} available",
            sizeof(uint64_t),
            heappages->size());
      } else {
        uint64_t pages = common::le_bytes_to_uint64(heappages->view());
        memory.resize(pages * kMemoryPageSize);
        log->trace(
            "Creating wasm module with non-default :heappages value set to {}",
            pages);
      }
    }

    instance.forDataSegment([&](auto offset, auto segment) {
      memory.storeBuffer(offset, segment);
    });

    return outcome::success();
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
          "Failed to obtain the block {} when initializing a runtime "
          "environment; Reason: {}",
          blockchain_state_,
          header_res.error());
      return Error::ABSENT_BLOCK;
    }

    OUTCOME_TRY(instance,
                parent_factory->module_repo_->getInstanceAt(
                    parent_factory->code_provider_,
                    blockchain_state_,
                    header_res.value()));

    const auto &env = instance->getEnvironment();
    if (persistent_) {
      if (auto res = env.storage_provider->setToPersistentAt(storage_state_);
          !res) {
        SL_DEBUG(
            parent_factory->logger_,
            "Failed to set the storage state to hash {:l} when initializing a "
            "runtime environment; Reason: {}",
            storage_state_,
            res.error());
        return Error::FAILED_TO_SET_STORAGE_STATE;
      }
    } else {
      if (auto res = env.storage_provider->setToEphemeralAt(storage_state_);
          !res) {
        SL_DEBUG(
            parent_factory->logger_,
            "Failed to set the storage state to hash {:l} when initializing a "
            "runtime environment; Reason: {}",
            storage_state_,
            res.error());
        return Error::FAILED_TO_SET_STORAGE_STATE;
      }
    }

    OUTCOME_TRY(resetMemory(*instance));

    SL_DEBUG(parent_factory->logger_,
             "Runtime environment at {}, state: {:l}",
             blockchain_state_,
             storage_state_);

    auto runtime_env = std::make_unique<RuntimeEnvironment>(
        instance, env.memory_provider, env.storage_provider, blockchain_state_);
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
          genesis_hash.error());
      return Error::ABSENT_BLOCK;
    }
    return start(genesis_hash.value());
  }

}  // namespace kagome::runtime
