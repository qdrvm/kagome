/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MERKLE_TRIE_HPP
#define KAGOME_MERKLE_TRIE_HPP

#include "common/buffer.hpp"
#include "storage/keyvalue.hpp"

namespace kagome::storage::merkle {

  /**
   * @brief This class represents a cryptographically authenticated key-value
   * storage - Merkle Trie DB backed by Key-Value database.
   */
  class TrieDb : public KeyValue {
   public:
    /**
     * @brief Calculate and return trie root.
     * @return byte buffer of any size (different hashing algos may be used)
     */
    virtual common::Buffer getRoot() const = 0;
  };

}  // namespace kagome::storage::merkle

#endif  // KAGOME_MERKLE_TRIE_HPP
