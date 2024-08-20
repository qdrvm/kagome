/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/common/core_api_factory_impl.hpp"

#include "runtime/common/uncompress_code_if_needed.hpp"
#include "runtime/heap_alloc_strategy_heappages.hpp"
#include "runtime/runtime_api/impl/core.hpp"
#include "runtime/runtime_context.hpp"
#include "runtime/runtime_instances_pool.hpp"
#include "runtime/trie_storage_provider.hpp"
#include "runtime/wabt/version.hpp"

namespace kagome::runtime {
  using primitives::Version;
  class GetVersion : public RestrictedCore {
   public:
    GetVersion(const Version &version) : version_{version} {}

    outcome::result<Version> version() {
      return version_;
    }

   private:
    Version version_;
  };

  CoreApiFactoryImpl::CoreApiFactoryImpl(
      std::shared_ptr<crypto::Hasher> hasher,
      LazySPtr<RuntimeInstancesPool> instance_pool)
      : hasher_{std::move(hasher)}, instance_pool_{std::move(instance_pool)} {}

  outcome::result<std::unique_ptr<RestrictedCore>> CoreApiFactoryImpl::make(
      BufferView code_zstd,
      std::shared_ptr<TrieStorageProvider> storage_provider) const {
    auto code_hash = hasher_->blake2b_256(code_zstd);
    OUTCOME_TRY(code, uncompressCodeIfNeeded(code_zstd));
    OUTCOME_TRY(version, readEmbeddedVersion(code));
    if (version) {
      return std::make_unique<GetVersion>(*version);
    }
    if (not instance_pool_.get()) {
      return std::errc::not_supported;
    }
    MemoryLimits config;
    BOOST_OUTCOME_TRY(config.heap_alloc_strategy,
                      heapAllocStrategyHeappagesDefault(
                          *storage_provider->getCurrentBatch()));
    OUTCOME_TRY(instance,
                instance_pool_.get()->instantiateFromCode(
                    code_hash,
                    [&] { return std::make_shared<Buffer>(code); },
                    {config}));
    OUTCOME_TRY(ctx, RuntimeContextFactory::stateless(instance));
    return std::make_unique<RestrictedCoreImpl>(std::move(ctx));
  }

}  // namespace kagome::runtime
