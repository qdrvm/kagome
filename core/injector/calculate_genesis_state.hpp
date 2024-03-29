/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "application/chain_spec.hpp"
#include "runtime/runtime_api/impl/core.hpp"
#include "storage/predefined_keys.hpp"
#include "storage/trie/polkadot_trie/polkadot_trie_impl.hpp"
#include "storage/trie/serialization/trie_serializer.hpp"
#include "storage/trie_pruner/trie_pruner.hpp"

namespace kagome::injector {

  inline outcome::result<storage::trie::RootHash> calculate_genesis_state(
      const application::ChainSpec &chain_spec,
      const runtime::ModuleFactory &module_factory,
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

    OUTCOME_TRY(runtime_version,
                runtime::callCoreVersion(module_factory, code, runtime_cache));
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
