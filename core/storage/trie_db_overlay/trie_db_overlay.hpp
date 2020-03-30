/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STORAGE_TRIE_DB_OVERLAY_HPP
#define KAGOME_STORAGE_TRIE_DB_OVERLAY_HPP

#include "storage/trie/trie_db.hpp"

namespace kagome::storage::trie_db_overlay {

  class TrieDbOverlay : public trie::TrieDb {
   public:
    virtual void commitAndSinkTo(ChangesTrie& changes_trie) = 0;

  };

}  // namespace kagome::storage::trie_db_overlay

#endif  // KAGOME_STORAGE_TRIE_DB_OVERLAY_HPP
