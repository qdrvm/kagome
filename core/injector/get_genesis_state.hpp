/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_INJECTOR_GET_GENESIS_STATE_HPP
#define KAGOME_CORE_INJECTOR_GET_GENESIS_STATE_HPP

#include "application/chain_spec.hpp"
#include "runtime/common/uncompress_code_if_needed.hpp"
#include "runtime/module.hpp"
#include "runtime/module_factory.hpp"
#include "runtime/runtime_api/impl/core.hpp"
#include "storage/predefined_keys.hpp"
#include "storage/trie/polkadot_trie/polkadot_trie_impl.hpp"
#include "storage/trie/serialization/trie_serializer.hpp"

namespace kagome::injector {
  inline outcome::result<storage::trie::RootHash> get_genesis_state(
      const application::ChainSpec &chain_spec,
      const runtime::ModuleFactory &module_factory,
      storage::trie::TrieSerializer &trie_serializer) {
    auto trie_from = [](const application::GenesisRawData &kv) {
      storage::trie::PolkadotTrieImpl trie;
      for (auto &[k, v] : kv) {
        trie.put(k, common::BufferView{v}).value();
      }
      return trie;
    };
    auto top_trie = trie_from(chain_spec.getGenesisTopSection());
    OUTCOME_TRY(code_zstd, top_trie.get(storage::kRuntimeCodeKey));
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
    OUTCOME_TRY(env.resetMemory());
    runtime::CoreImpl core_api{
        std::make_shared<runtime::Executor>(nullptr, nullptr),
        nullptr,
        nullptr,
    };
    OUTCOME_TRY(runtime_version, core_api.version(env));
    auto version = storage::trie::StateVersion{runtime_version.state_version};
    for (auto &[child, kv] : chain_spec.getGenesisChildrenDefaultSection()) {
      auto trie = trie_from(kv);
      OUTCOME_TRY(root, trie_serializer.storeTrie(trie, version));
      common::Buffer child2;
      child2 += storage::kChildStorageDefaultPrefix;
      child2 += child;
      top_trie.put(child2, common::BufferView{root}).value();
    }
    return trie_serializer.storeTrie(top_trie, version);
  }
}  // namespace kagome::injector

#endif  // KAGOME_CORE_INJECTOR_GET_GENESIS_STATE_HPP
