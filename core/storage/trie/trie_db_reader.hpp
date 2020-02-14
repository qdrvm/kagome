/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TRIE_DB_READER_HPP
#define KAGOME_TRIE_DB_READER_HPP

#include "common/buffer.hpp"
#include "storage/face/readable.hpp"

namespace kagome::storage::trie {

  /**
   * Provides read access for Trie DB - cryptographically authenticated
   * key-value storage
   */
  class TrieDbReader : public face::ReadOnlyMap<Buffer, Buffer> {
   public:
    /**
     * @brief Calculate and return trie root.
     * @return byte buffer of any size (different hashing algorithms may be
     * used)
     */
    virtual Buffer getRootHash() const = 0;

    /**
     * @returns true if the trie is empty, false otherwise
     */
    virtual bool empty() const = 0;
  };

}  // namespace kagome::storage::trie

#endif  // KAGOME_TRIE_DB_READER_HPP
