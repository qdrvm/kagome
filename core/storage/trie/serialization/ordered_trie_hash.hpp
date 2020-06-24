/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_ORDERED_TRIE_HASH_HPP
#define KAGOME_ORDERED_TRIE_HASH_HPP

#include "common/buffer.hpp"
#include "crypto/hasher.hpp"
#include "scale/scale.hpp"
#include "storage/trie/polkadot_trie/polkadot_trie_impl.hpp"
#include "storage/trie/serialization/polkadot_codec.hpp"

namespace kagome::storage::trie {

  /**
   * Calculates the hash of a Merkle tree containing the items from the provided
   * range [begin; end) as values and compact-encoded indices of those
   * values(starting from 0) as keys
   * @tparam It an iterator type of a container of common::Buffers
   * @return the Merkle tree root hash of the tree containing provided values
   */
  template <typename It>
  outcome::result<common::Buffer> calculateOrderedTrieHash(const It &begin,
                                                           const It &end) {
    PolkadotTrieImpl trie;
    PolkadotCodec codec;
    // empty root
    if (begin == end) {
      static const auto empty_root = common::Buffer{}.put(codec.hash256({0}));
      return empty_root;
    }
    // clang-format off
    static_assert(
        std::is_same_v<std::decay_t<decltype(*begin)>, common::Buffer>);
    // clang-format on
    It it = begin;
    scale::CompactInteger key = 0;
    while (it != end) {
      OUTCOME_TRY(enc, scale::encode(key++));
      OUTCOME_TRY(trie.put(common::Buffer{enc}, *it));
      it++;
    }
    OUTCOME_TRY(enc, codec.encodeNode(*trie.getRoot()));
    return common::Buffer{codec.hash256(enc)};
  }

}  // namespace kagome::storage::trie

#endif  // KAGOME_ORDERED_TRIE_HASH_HPP
