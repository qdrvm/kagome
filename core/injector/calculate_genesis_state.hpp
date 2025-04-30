/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "application/chain_spec.hpp"
#include "runtime/common/uncompress_code_if_needed.hpp"
#include "runtime/heap_alloc_strategy_heappages.hpp"
#include "runtime/runtime_api/impl/core.hpp"
#include "runtime/runtime_instances_pool.hpp"
#include "runtime/wabt/version.hpp"
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
      storage::BufferStorage &direct_trie_storage,
      std::shared_ptr<runtime::RuntimePropertiesCache> runtime_cache) {
    auto trie_from = [&direct_trie_storage](BufferView prefix,
                        const application::GenesisRawData &kv)
        -> outcome::result<std::shared_ptr<storage::trie::PolkadotTrieImpl>> {
      auto trie = storage::trie::PolkadotTrieImpl::createEmpty();
      for (const auto &[key, val] : kv) {
        OUTCOME_TRY(trie->put(Buffer{prefix}.put(key), val.view()));
        OUTCOME_TRY(
            direct_trie_storage.put(Buffer{prefix}.put(key), val.view()));
      }
      return trie;
    };
    OUTCOME_TRY(top_trie, trie_from({}, chain_spec.getGenesisTopSection()));
    OUTCOME_TRY(code, top_trie->get(storage::kRuntimeCodeKey));

    auto code_hash = hasher.blake2b_256(code);
    BOOST_OUTCOME_TRY(code, runtime::uncompressCodeIfNeeded(code));
    OUTCOME_TRY(runtime_version, runtime::readEmbeddedVersion(code));
    if (not runtime_version) {
      runtime::MemoryLimits config;
      BOOST_OUTCOME_TRY(config.heap_alloc_strategy,
                        heapAllocStrategyHeappagesDefault(*top_trie));
      OUTCOME_TRY(instance,
                  module_factory.instantiateFromCode(
                      code_hash,
                      [&] { return std::make_shared<Buffer>(code); },
                      runtime::RuntimeContext::ContextParams{config, {}, {}}));
      OUTCOME_TRY(ctx, runtime::RuntimeContextFactory::stateless(instance));
      auto version_res =
          runtime_cache->getVersion(ctx.module_instance->getCodeHash(), [&] {
            return ctx.module_instance
                ->callAndDecodeExportFunction<primitives::Version>(
                    ctx, "Core_version");
          });
      OUTCOME_TRY(version_res);
      runtime_version = std::move(version_res.value());
    }
    auto version = storage::trie::StateVersion{runtime_version->state_version};
    std::vector<std::shared_ptr<storage::trie::PolkadotTrie>> child_tries;
    for (const auto &[child, kv] :
         chain_spec.getGenesisChildrenDefaultSection()) {
      common::Buffer child_prefix;
      child_prefix += storage::kChildStorageDefaultPrefix;
      child_prefix += child;

      OUTCOME_TRY(child_trie, trie_from(child_prefix, kv));
      child_tries.emplace_back(child_trie);
      OUTCOME_TRY(root_and_batch,
                  trie_serializer.storeTrie(*child_tries.back(), version));
      OUTCOME_TRY(root_and_batch.second->commit());

      OUTCOME_TRY(top_trie->put(child_prefix,
                                common::BufferView{root_and_batch.first}));
    }

    OUTCOME_TRY(root_and_batch, trie_serializer.storeTrie(*top_trie, version));
    OUTCOME_TRY(root_and_batch.second->commit());
    OUTCOME_TRY(direct_trie_storage.put(storage::trie::kLastCommittedHashKey,
                                        root_and_batch.first));
    return root_and_batch.first;
  }
}  // namespace kagome::injector
