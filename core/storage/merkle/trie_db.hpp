/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MERKLE_TRIE_HPP
#define KAGOME_MERKLE_TRIE_HPP

#include "common/buffer.hpp"
#include "storage/keyvalue.hpp"

namespace kagome::storage::merkle {

  class TrieDb : public KeyValue {
   public:
    virtual common::Buffer getRoot() const = 0;
  };

}  // namespace kagome::storage

#endif  // KAGOME_MERKLE_TRIE_HPP
