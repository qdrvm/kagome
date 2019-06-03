/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MERKLE_TRIE_HPP
#define KAGOME_MERKLE_TRIE_HPP

#include "common/buffer.hpp"
#include "storage/buffer_map.hpp"

namespace kagome::storage::merkle {

  /**
   * @brief This class represents a cryptographically authenticated key-value
   * storage - Merkle Trie DB backed by Key-Value database.
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
     * @param buf
     */
    virtual void clearPrefix(const common::Buffer &buf) = 0;
  };

}  // namespace kagome::storage::merkle

#endif  // KAGOME_MERKLE_TRIE_HPP
