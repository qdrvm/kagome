/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_API_STATE_READONLY_TRIE_BUILDER_HPP
#define KAGOME_API_STATE_READONLY_TRIE_BUILDER_HPP

#include "primitives/common.hpp"
#include "storage/trie/trie_db_reader.hpp"

namespace kagome::api {

  /**
   * A builder class for initializing a read-only trie at a specific point in
   * the chain (basically restoring the state at that point)
   */
  class ReadonlyTrieBuilder {
   public:
    virtual ~ReadonlyTrieBuilder() = default;

    virtual std::unique_ptr<storage::trie::TrieDbReader> buildAt(
        primitives::BlockHash state_root) const = 0;
  };

}  // namespace kagome::api

#endif  // KAGOME_API_STATE_READONLY_TRIE_BUILDER_HPP
