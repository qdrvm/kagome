/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STORAGE_TRIE_DB_FACTORY_HPP
#define KAGOME_STORAGE_TRIE_DB_FACTORY_HPP

#include "storage/trie/trie_db.hpp"

namespace kagome::storage::trie {

  /**
   * An abstract factory that creates trie db instances
   */
  class TrieDbFactory {
   public:
    virtual ~TrieDbFactory() = default;

    virtual std::unique_ptr<TrieDb> makeTrieDb() const = 0;
  };

}  // namespace kagome::storage::trie

#endif  // KAGOME_STORAGE_TRIE_DB_FACTORY_HPP
