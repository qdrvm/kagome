/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KAGOME_ORDERED_TRIE_HASH_HPP
#define KAGOME_ORDERED_TRIE_HASH_HPP

#include "common/buffer.hpp"
#include "crypto/hasher.hpp"
#include "scale/scale.hpp"
#include "storage/trie/impl/polkadot_codec.hpp"
#include "storage/trie/impl/polkadot_trie.hpp"

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
    PolkadotTrie trie;
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
