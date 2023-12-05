/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/buffer.hpp"
#include "crypto/hasher.hpp"
#include "scale/scale.hpp"
#include "storage/trie/polkadot_trie/polkadot_trie_impl.hpp"
#include "storage/trie/serialization/polkadot_codec.hpp"
#include "storage/trie/types.hpp"

namespace kagome::storage::trie {

  /**
   * Calculates the hash of a Merkle tree containing the items from the provided
   * range [begin; end) as values and compact-encoded indices of those
   * values(starting from 0) as keys
   * @tparam It an iterator type of a container of common::Buffers
   * @return the Merkle tree root hash of the tree containing provided values
   */
  template <typename It>
  outcome::result<RootHash> calculateOrderedTrieHash(StateVersion version,
                                                     const It &begin,
                                                     const It &end) {
    auto trie = storage::trie::PolkadotTrieImpl::createEmpty();
    PolkadotCodec codec;
    // empty root
    if (begin == end) {
      return kEmptyRootHash;
    }
    static_assert(
        std::is_same_v<std::decay_t<decltype(*begin)>, common::Buffer>);
    It it = begin;
    scale::CompactInteger key = 0;
    while (it != end) {
      OUTCOME_TRY(enc, scale::encode(key++));
      OUTCOME_TRY(trie->put(enc, BufferView{*it}));
      it++;
    }
    OUTCOME_TRY(enc, codec.encodeNode(*trie->getRoot(), version, {}));
    return codec.hash256(enc);
  }

  template <typename ContainerType>
  inline outcome::result<RootHash> calculateOrderedTrieHash(
      StateVersion version, const ContainerType &container) {
    return calculateOrderedTrieHash(
        version, container.begin(), container.end());
  }

}  // namespace kagome::storage::trie
