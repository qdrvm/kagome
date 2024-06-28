/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "core_api_factory_impl.hpp"

#include "runtime/heap_alloc_strategy_heappages.hpp"
#include "runtime/runtime_api/impl/core.hpp"
#include "runtime/runtime_context.hpp"
#include "runtime/runtime_instances_pool.hpp"
#include "runtime/trie_storage_provider.hpp"

namespace kagome::runtime {

  CoreApiFactoryImpl::CoreApiFactoryImpl(
      std::shared_ptr<crypto::Hasher> hasher,
      LazySPtr<RuntimeInstancesPool> module_factory)
      : hasher_{std::move(hasher)},
        module_factory_{std::move(module_factory)} {}

  outcome::result<std::unique_ptr<RestrictedCore>> CoreApiFactoryImpl::make(
      BufferView code_zstd,
      std::shared_ptr<TrieStorageProvider> storage_provider) const {
    auto code_hash = hasher_->blake2b_256(code_zstd);
    // TODO(turuslan): #2139, read_embedded_version
    MemoryLimits config;
    BOOST_OUTCOME_TRY(config.heap_alloc_strategy,
                      heapAllocStrategyHeappagesDefault(
                          *storage_provider->getCurrentBatch()));
    OUTCOME_TRY(instance,
                module_factory_.get()->instantiateFromCode(
                    code_hash,
                    [&] { return std::make_shared<Buffer>(code_zstd); },
                    {config}));
    OUTCOME_TRY(ctx, RuntimeContextFactory::fromCode(instance));
    return std::make_unique<RestrictedCoreImpl>(std::move(ctx));
  }

}  // namespace kagome::runtime
