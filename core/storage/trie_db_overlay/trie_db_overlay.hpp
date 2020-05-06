/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STORAGE_TRIE_DB_OVERLAY_HPP
#define KAGOME_STORAGE_TRIE_DB_OVERLAY_HPP

#include "storage/changes_trie/changes_trie_builder.hpp"
#include "storage/trie/trie_db.hpp"

namespace kagome::storage::trie_db_overlay {

  class TrieDbOverlay : public trie::TrieDb {
   public:
    // commit all operations pending in memory to the main storage
    virtual outcome::result<void> commit() = 0;

    // insert all accumulated changes to the provided changes trie
    // clear the accumulated changes set
    virtual outcome::result<void> sinkChangesTo(
        storage::changes_trie::ChangesTrieBuilder &changes_trie) = 0;
  };

}  // namespace kagome::storage::trie_db_overlay

#endif  // KAGOME_STORAGE_TRIE_DB_OVERLAY_HPP
