/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_STATE_SYNC_REQUEST_FLOW_HPP
#define KAGOME_NETWORK_STATE_SYNC_REQUEST_FLOW_HPP

#include <unordered_map>
#include <unordered_set>

#include "network/types/state_request.hpp"
#include "network/types/state_response.hpp"
#include "primitives/block_header.hpp"
#include "storage/trie/polkadot_trie/polkadot_trie_impl.hpp"

namespace kagome::runtime {
  class Core;
  class ModuleFactory;
}  // namespace kagome::runtime

namespace kagome::storage::trie {
  class TrieSerializer;
}  // namespace kagome::storage::trie

namespace kagome::storage::trie_pruner {
  class TriePruner;
}  // namespace kagome::storage::trie_pruner

namespace kagome::primitives {
  struct BlockHeader;
}  // namespace kagome::primitives

namespace kagome::network {
  /**
   * https://github.com/paritytech/substrate/blob/master/client/network/sync/src/state.rs
   */
  class StateSyncRequestFlow {
    using RootHash = storage::trie::RootHash;
    struct Root {
      std::shared_ptr<storage::trie::PolkadotTrie> trie =
          storage::trie::PolkadotTrieImpl::createEmpty();
      std::vector<common::Buffer> children;
    };

   public:
    enum class Error {
      EMPTY_RESPONSE = 1,
      HASH_MISMATCH,
    };

    StateSyncRequestFlow(
        std::shared_ptr<storage::trie_pruner::TriePruner> state_pruner,
        const primitives::BlockInfo &block_info,
        const primitives::BlockHeader &block);

    auto &blockInfo() const {
      return block_info_;
    }

    bool complete() const;

    StateRequest nextRequest() const;

    outcome::result<void> onResponse(const StateResponse &res);

    outcome::result<void> commit(
        const runtime::ModuleFactory &module_factory,
        runtime::Core &core_api,
        storage::trie::TrieSerializer &trie_serializer);

   private:
    std::shared_ptr<storage::trie_pruner::TriePruner> state_pruner_;

    primitives::BlockInfo block_info_;
    primitives::BlockHeader block_;

    std::vector<common::Buffer> last_key_;
    std::unordered_map<std::optional<RootHash>, Root> roots_;
    std::unordered_set<std::optional<RootHash>> complete_roots_;
  };
}  // namespace kagome::network

OUTCOME_HPP_DECLARE_ERROR(kagome::network, StateSyncRequestFlow::Error)

#endif  //  KAGOME_NETWORK_STATE_SYNC_REQUEST_FLOW_HPP
