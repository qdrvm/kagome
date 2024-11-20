/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <outcome/outcome.hpp>

namespace kagome {
  namespace blockchain {
    class BlockTree;
  }
  namespace storage::trie {
    class TrieStorage;
    class TrieStorageBackend;
  }  // namespace storage::trie
}  // namespace kagome

namespace kagome::storage::migrations {

  outcome::result<void> separateTrieValues(
      const blockchain::BlockTree &block_tree,
      const trie::TrieStorage &trie_storage,
      trie::TrieStorageBackend &trie_backend);

}  // namespace kagome::storage::migrations
