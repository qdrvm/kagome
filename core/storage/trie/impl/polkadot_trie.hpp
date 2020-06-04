/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STORAGE_TRIE_POLKADOT_TRIE_HPP
#define KAGOME_STORAGE_TRIE_POLKADOT_TRIE_HPP

#include "storage/face/generic_maps.hpp"

#include "storage/trie/impl/polkadot_node.hpp"

namespace kagome::storage::trie {

  /**
   * For specification see Polkadot Runtime Environment Protocol Specification
   * '2.1.2 The General Tree Structure' and further
   */
  class PolkadotTrie : public face::Readable<common::Buffer, common::Buffer>,
                       public face::Writeable<common::Buffer, common::Buffer> {
   public:
    using NodePtr = std::shared_ptr<PolkadotNode>;
    using BranchPtr = std::shared_ptr<BranchNode>;

    /**
     * Remove all trie entries which key begins with the supplied prefix
     */
    virtual outcome::result<void> clearPrefix(
        const common::Buffer &prefix) = 0;

    /**
     * @return the root node of the trie
     */
    virtual NodePtr getRoot() const = 0;
  };

}  // namespace kagome::storage::trie

#endif  // KAGOME_STORAGE_TRIE_POLKADOT_TRIE_HPP
