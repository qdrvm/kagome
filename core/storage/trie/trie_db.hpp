/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TRIE_TRIE_DB_HPP
#define KAGOME_TRIE_TRIE_DB_HPP

#include "common/buffer.hpp"
#include "storage/buffer_map_types.hpp"
#include "storage/trie/trie_db_reader.hpp"

namespace kagome::storage::trie {

  /**
   * @brief Represents a cryptographically authenticated key-value
   * storage - Trie DB, backed by a key-value database.
   */
  class TrieDb : public TrieDbReader, public BatchWriteBufferMap {
   public:
    /**
     * remove storage entries which keys start with given prefix
     */
    virtual outcome::result<void> clearPrefix(const common::Buffer &buf) = 0;
  };

}  // namespace kagome::storage::trie

#endif  // KAGOME_TRIE_TRIE_DB_HPP
