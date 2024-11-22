/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <outcome/outcome.hpp>
#include "injector/application_injector.hpp"

namespace kagome {
  namespace blockchain {
    class BlockTree;
  }
  namespace storage::trie {
    class TrieStorage;
    class TrieStorageBackend;
  }  // namespace storage::trie
  namespace injector {
    class KagomeNodeInjector;
  }
}  // namespace kagome

namespace kagome::storage::migrations {

  outcome::result<void> separateTrieValues(
      const blockchain::BlockTree &block_tree,
      const trie::TrieStorage &trie_storage,
      trie::TrieStorageBackend &trie_backend);

  outcome::result<void> runMigrations(injector::KagomeNodeInjector &injector);

}  // namespace kagome::storage::migrations
