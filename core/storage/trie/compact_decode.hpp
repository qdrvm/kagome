/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STORAGE_TRIE_COMPACT_DECODE_HPP
#define KAGOME_STORAGE_TRIE_COMPACT_DECODE_HPP

#include <unordered_map>

#include "common/blob.hpp"
#include "common/buffer.hpp"

namespace kagome::storage::trie {
  struct TrieNode;

  enum class CompactDecodeError {
    INCOMPLETE_PROOF = 1,
  };

  using CompactDecoded =
      std::unordered_map<common::Hash256,
                         std::pair<common::Buffer, std::shared_ptr<TrieNode>>>;

  outcome::result<CompactDecoded> compactDecode(common::BufferView raw_proof);
}  // namespace kagome::storage::trie

OUTCOME_HPP_DECLARE_ERROR(kagome::storage::trie, CompactDecodeError)

#endif  // KAGOME_STORAGE_TRIE_COMPACT_DECODE_HPP
