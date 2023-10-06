/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STORAGE_TRIE_COMPACT_ENCODE_HPP
#define KAGOME_STORAGE_TRIE_COMPACT_ENCODE_HPP

#include "storage/trie/on_read.hpp"

namespace kagome::storage::trie {
  outcome::result<common::Buffer> compactEncode(const OnRead &db,
                                                const common::Hash256 &root);
}  // namespace kagome::storage::trie

#endif  // KAGOME_STORAGE_TRIE_COMPACT_ENCODE_HPP
