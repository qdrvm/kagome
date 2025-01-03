/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <unordered_set>

#include "log/logger.hpp"
#include "network/types/state_request.hpp"
#include "network/types/state_response.hpp"
#include "primitives/block_header.hpp"
#include "storage/trie/raw_cursor.hpp"

namespace kagome::storage::trie {
  class TrieStorageBackend;
}  // namespace kagome::storage::trie

namespace kagome::network {
  /**
   * Recursive coroutine to fetch missing trie nodes with "/state/2" protocol.
   *
   * https://github.com/paritytech/substrate/blob/master/client/network/sync/src/state.rs
   */
  class StateSyncRequestFlow {
   public:
    struct Item {
      common::Hash256 hash;
      common::Buffer encoded;
    };

    using Level = storage::trie::RawCursor<Item>;

    StateSyncRequestFlow(
        std::shared_ptr<storage::trie::TrieStorageBackend> node_db,
        const primitives::BlockInfo &block_info,
        const primitives::BlockHeader &block);

    auto &blockInfo() const {
      return block_info_;
    }

    auto &root() const {
      return block_.state_root;
    }

    bool complete() const {
      return done_;
    }

    StateRequest nextRequest() const;

    outcome::result<void> onResponse(const StateResponse &res);

   private:
    bool isKnown(const common::Hash256 &hash);

    std::shared_ptr<storage::trie::TrieStorageBackend> node_db_;

    primitives::BlockInfo block_info_;
    primitives::BlockHeader block_;

    std::vector<Level> levels_;
    std::unordered_set<common::Hash256> known_;

    size_t stat_count_ = 0, stat_size_ = 0;

    bool done_ = false;

    log::Logger log_;
  };
}  // namespace kagome::network
