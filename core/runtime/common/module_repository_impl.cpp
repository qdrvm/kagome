/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/common/module_repository_impl.hpp"

#include "blockchain/block_header_repository.hpp"
#include "log/profiling_logger.hpp"
#include "runtime/common/runtime_instances_pool.hpp"
#include "runtime/common/uncompress_code_if_needed.hpp"
#include "runtime/heap_alloc_strategy_heappages.hpp"
#include "runtime/instance_environment.hpp"
#include "runtime/module.hpp"
#include "runtime/module_factory.hpp"
#include "runtime/module_instance.hpp"
#include "runtime/runtime_code_provider.hpp"
#include "runtime/runtime_upgrade_tracker.hpp"
#include "runtime/wabt/version.hpp"
#include "storage/trie/trie_storage.hpp"

namespace kagome::runtime {
  using kagome::primitives::ThreadNumber;
  using soralog::util::getThreadNumber;

  ModuleRepositoryImpl::ModuleRepositoryImpl(
      std::shared_ptr<RuntimeInstancesPool> runtime_instances_pool,
      std::shared_ptr<crypto::Hasher> hasher,
      std::shared_ptr<blockchain::BlockHeaderRepository>
          block_header_repository,
      std::shared_ptr<RuntimeUpgradeTracker> runtime_upgrade_tracker,
      std::shared_ptr<storage::trie::TrieStorage> trie_storage,
      std::shared_ptr<const ModuleFactory> module_factory,
      std::shared_ptr<const RuntimeCodeProvider> code_provider)
      : runtime_instances_pool_{std::move(runtime_instances_pool)},
        hasher_{std::move(hasher)},
        block_header_repository_{std::move(block_header_repository)},
        runtime_upgrade_tracker_{std::move(runtime_upgrade_tracker)},
        trie_storage_{std::move(trie_storage)},
        module_factory_{std::move(module_factory)},
        code_provider_{std::move(code_provider)},
        cache_{4},
        logger_{log::createLogger("Module Repository", "runtime")} {
    BOOST_ASSERT(runtime_instances_pool_);
    BOOST_ASSERT(runtime_upgrade_tracker_);
    BOOST_ASSERT(module_factory_);
    BOOST_ASSERT(code_provider_);
  }

  outcome::result<std::shared_ptr<ModuleInstance>>
  ModuleRepositoryImpl::getInstanceAt(
      const primitives::BlockInfo &block,
      const storage::trie::RootHash &storage_state) {
    OUTCOME_TRY(item, codeAt(block, storage_state));
    return runtime_instances_pool_->instantiateFromCode(
        item.hash, [&] { return item.code; }, item.ctx_params);
  }

  outcome::result<std::optional<primitives::Version>>
  ModuleRepositoryImpl::embeddedVersion(
      const primitives::BlockHash &block_hash) {
    OUTCOME_TRY(header, block_header_repository_->getBlockHeader(block_hash));
    OUTCOME_TRY(item, codeAt(header.blockInfo(), header.state_root));
    return item.version;
  }

  outcome::result<ModuleRepositoryImpl::Item> ModuleRepositoryImpl::codeAt(
      const primitives::BlockInfo &block,
      const storage::trie::RootHash &storage_state) {
    KAGOME_PROFILE_START(code_retrieval)
    OUTCOME_TRY(state, runtime_upgrade_tracker_->getLastCodeUpdateState(block));
    KAGOME_PROFILE_END(code_retrieval)

    KAGOME_PROFILE_START(module_retrieval)
    Item item;
    auto res = SAFE_UNIQUE(cache_)->outcome::result<void> {
      if (auto r = cache_.get(state)) {
        item = r->get();
      } else {
        auto code_res = code_provider_->getCodeAt(state);
        if (not code_res) {
          code_res = code_provider_->getCodeAt(storage_state);
        }
        auto &code_zstd = *code_res.value();
        item.hash = hasher_->blake2b_256(code_zstd);
        OUTCOME_TRY(code, uncompressCodeIfNeeded(code_zstd));
        item.code = std::make_shared<Buffer>(code);
        BOOST_OUTCOME_TRY(item.version, readEmbeddedVersion(code));
        OUTCOME_TRY(batch, trie_storage_->getEphemeralBatchAt(storage_state));
        BOOST_OUTCOME_TRY(item.ctx_params.memory_limits.heap_alloc_strategy,
                          heapAllocStrategyHeappagesDefault(*batch));

        item.ctx_params.optimization_level =
            DEFAULT_RELAY_CHAIN_RUNTIME_OPT_LEVEL;
        cache_.put(state, item);
      }
      return outcome::success();
    };
    OUTCOME_TRY(res);
    return item;
  }
}  // namespace kagome::runtime
