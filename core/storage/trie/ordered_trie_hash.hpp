/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_ORDERED_TRIE_HASH_HPP
#define KAGOME_ORDERED_TRIE_HASH_HPP

#include "common/buffer.hpp"
#include "storage/trie/trie_db.hpp"

namespace kagome::storage::trie {

  template <typename It>
  common::Hash256 calculateOrderedTrieHash(const It &begin, const It &end) {
    ;
    static_assert(
        std::is_same_v<
            It::value_type,
            common::
                Buffer> or std::is_same_v<std::decay_t<decltype(*begin)>, common::Buffer>);
    auto batch = trie.batch();
    It it = begin;
    while (it++ != end) {
      batch->put(*it);
    }
    batch->commit();

    return trie.getRootHash();
  }

}  // namespace kagome::storage::trie

#endif  // KAGOME_ORDERED_TRIE_HASH_HPP
