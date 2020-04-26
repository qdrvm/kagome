/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STORAGE_TRIE_READONLY_TRIE_FACTORY_HPP
#define KAGOME_STORAGE_TRIE_READONLY_TRIE_FACTORY_HPP

#include "primitives/common.hpp"
#include "storage/trie/readonly_trie_factory.hpp"

namespace kagome::storage::trie {

  /**
   * A factory class for initializing a read-only trie at a specific point in
   * the chain (basically restoring the state at that point)
   */
  class ReadonlyTrieFactory {
   public:
    virtual ~ReadonlyTrieFactory() = default;

    virtual std::unique_ptr<storage::trie::TrieDbReader> buildAt(
        primitives::BlockHash state_root) const = 0;
  };

}  // kagome::storage::trie

#endif  // KAGOME_STORAGE_TRIE_READONLY_TRIE_FACTORY_HPP
