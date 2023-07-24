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
#include "storage/trie/child_prefix.hpp"
#include "storage/trie/polkadot_trie/trie_node.hpp"

namespace kagome::storage::trie {
  class TrieStorageBackend;
}  // namespace kagome::storage::trie

namespace kagome::storage::trie_pruner {
  class TriePruner;
}  // namespace kagome::storage::trie_pruner

namespace kagome::primitives {
  struct BlockHeader;
}  // namespace kagome::primitives

namespace kagome::network {
  /**
   * Recursive coroutine to fetch missing trie nodes with "/state/2" protocol.
   *
   * https://github.com/paritytech/substrate/blob/master/client/network/sync/src/state.rs
   */
  class StateSyncRequestFlow {
   public:
    enum class Error {
      EMPTY_RESPONSE = 1,
      HASH_MISMATCH,
    };

    struct Item {
      common::Hash256 hash;
      common::Buffer encoded;
      std::shared_ptr<storage::trie::TrieNode> node;
      std::optional<uint8_t> branch;
      storage::trie::ChildPrefix child;
    };

    struct Level {
      common::Hash256 root;
      std::vector<Item> stack;
    };

    StateSyncRequestFlow(
        std::shared_ptr<storage::trie_pruner::TriePruner> state_pruner,
        std::shared_ptr<storage::trie::TrieStorageBackend> db,
        const primitives::BlockInfo &block_info,
        const primitives::BlockHeader &block);

    auto &blockInfo() const {
      return block_info_;
    }

    bool complete() const;

    StateRequest nextRequest() const;

    outcome::result<void> onResponse(const StateResponse &res);

    outcome::result<void> commit();

   private:
    bool isKnown(const common::Hash256 &hash);

    std::shared_ptr<storage::trie_pruner::TriePruner> state_pruner_;
    std::shared_ptr<storage::trie::TrieStorageBackend> db_;

    primitives::BlockInfo block_info_;
    primitives::BlockHeader block_;

    std::vector<Level> levels_;
    std::unordered_set<common::Hash256> known_;
    std::vector<common::Hash256> child_roots_;
  };
}  // namespace kagome::network

OUTCOME_HPP_DECLARE_ERROR(kagome::network, StateSyncRequestFlow::Error)

#endif  //  KAGOME_NETWORK_STATE_SYNC_REQUEST_FLOW_HPP
