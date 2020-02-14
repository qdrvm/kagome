/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TRIE_DB_READER_HPP
#define KAGOME_TRIE_DB_READER_HPP

#include "common/buffer.hpp"
#include "storage/face/readable_map.hpp"

namespace kagome::storage::trie {

  class TrieDbReader
      : virtual public face::ReadableMap<common::Buffer, common::Buffer>,
        virtual public face::IterableMap<common::Buffer, common::Buffer> {
   public:
    /**
     * @brief Calculate and return trie root.
     * @return byte buffer of any size (different hashing algorithms may be
     * used)
     */
    virtual common::Buffer getRootHash() const = 0;

    /**
     * @returns true if the trie is empty, false otherwise
     */
    virtual bool empty() const = 0;
  };

}  // namespace kagome::storage::trie

#endif  // KAGOME_TRIE_DB_READER_HPP
