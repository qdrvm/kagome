/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "application/chain_spec.hpp"
#include "runtime/heap_alloc_strategy_heappages.hpp"
#include "runtime/runtime_api/impl/core.hpp"
#include "runtime/runtime_instances_pool.hpp"
#include "storage/predefined_keys.hpp"
#include "storage/trie/polkadot_trie/polkadot_trie_impl.hpp"
#include "storage/trie/serialization/trie_serializer.hpp"
#include "storage/trie_pruner/trie_pruner.hpp"

namespace kagome::injector {

  inline outcome::result<storage::trie::RootHash> calculate_genesis_state(
      const application::ChainSpec &chain_spec,
      const crypto::Hasher &hasher,
      runtime::RuntimeInstancesPool &module_factory,
      storage::trie::TrieSerializer &trie_serializer,
      std::shared_ptr<runtime::RuntimePropertiesCache> runtime_cache) {
    auto trie_from = [](const application::GenesisRawData &kv) {
      auto trie = storage::trie::PolkadotTrieImpl::createEmpty();
      for (auto &[k, v] : kv) {
        trie->put(k, common::BufferView{v}).value();
      }
      return trie;
    };
    auto top_trie = trie_from(chain_spec.getGenesisTopSection());
    OUTCOME_TRY(code, top_trie->get(storage::kRuntimeCodeKey));

    runtime::MemoryLimits config;
    BOOST_OUTCOME_TRY(config.heap_alloc_strategy,
                      heapAllocStrategyHeappagesDefault(*top_trie));
    auto code_hash = hasher.blake2b_256(code);
    OUTCOME_TRY(instance,
                module_factory.instantiateFromCode(
                    code_hash,
                    [&] { return std::make_shared<Buffer>(code); },
                    {config}));
    OUTCOME_TRY(ctx, runtime::RuntimeContextFactory::fromCode(instance));
    OUTCOME_TRY(runtime_version,
                runtime_cache->getVersion(
                    ctx.module_instance->getCodeHash(),
                    [&]() -> outcome::result<primitives::Version> {
                      return ctx.module_instance
                          ->callAndDecodeExportFunction<primitives::Version>(
                              ctx, "Core_version");
                    }));
    auto version = storage::trie::StateVersion{runtime_version.state_version};
    std::vector<std::shared_ptr<storage::trie::PolkadotTrie>> child_tries;
    for (auto &[child, kv] : chain_spec.getGenesisChildrenDefaultSection()) {
      child_tries.emplace_back(trie_from(kv));
      OUTCOME_TRY(root,
                  trie_serializer.storeTrie(*child_tries.back(), version));

      common::Buffer child2;
      child2 += storage::kChildStorageDefaultPrefix;
      child2 += child;
      top_trie->put(child2, common::BufferView{root}).value();
    }

    OUTCOME_TRY(trie_hash, trie_serializer.storeTrie(*top_trie, version));

    return trie_hash;
  }
}  // namespace kagome::injector
