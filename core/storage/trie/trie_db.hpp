/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TRIE_TRIE_DB_HPP
#define KAGOME_TRIE_TRIE_DB_HPP

#include "common/buffer.hpp"
#include "storage/buffer_map.hpp"

namespace kagome::storage::trie {

  /**
   * @brief This class represents a cryptographically authenticated key-value
   * storage - Trie DB backed by Key-Value database.
   */
  class TrieDb : public PersistentBufferMap {
   public:
    /**
     * @brief Calculate and return trie root.
     * @return byte buffer of any size (different hashing algorithms may be used)
     */
    virtual common::Buffer getRootHash() const = 0;

    /**
     * remove storage entries which keys start with given prefix
     */
    virtual outcome::result<void> clearPrefix(const common::Buffer &buf) = 0;

    /**
     * @returns true if the trie is empty, false otherwise
     */
    virtual bool empty() const = 0;
  };

}  // namespace kagome::storage::trie

#endif  // KAGOME_TRIE_TRIE_DB_HPP
