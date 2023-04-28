/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TRIEPRUNER_HPP
#define KAGOME_TRIEPRUNER_HPP

#include <vector>

#include "common/buffer.hpp"
#include "primitives/block_header.hpp"
#include "primitives/block_id.hpp"
#include "storage/trie/types.hpp"

namespace kagome::storage::trie {
  class PolkadotTrie;
}

namespace kagome::storage::trie_pruner {

  /**
   * Pruner is responsible for removal of trie storage parts belonging to
   * old or discarded blocks from the database. It works in real-time.
   */
  class TriePruner {
   public:
    virtual ~TriePruner() = default;

    /**
     * Register a new trie with the trie pruner so that the trie nodes
     * this trie references are kept until the block this trie belongs to is
     * pruned.
     * @param version trie version used by runtime when creating this trie.
     */
    virtual outcome::result<void> addNewState(
        trie::PolkadotTrie const &new_trie, trie::StateVersion version) = 0;

    /**
     * Register a new child trie with the trie pruner so that the trie nodes
     * this trie references are kept until the block this trie belongs to is
     * pruned.
     * @param version trie version used by runtime when creating this trie.
     */
    virtual outcome::result<void> addNewChildState(
        storage::trie::RootHash const &parent_root,
        common::Buffer const &key,
        trie::PolkadotTrie const &new_trie,
        trie::StateVersion version) = 0;

    /// wrapper for RootHash to avoid confusing parameter order
    struct Parent {
      explicit Parent(storage::trie::RootHash const &hash) : hash{hash} {}
      storage::trie::RootHash const &hash;
    };

    /// wrapper for RootHash to avoid confusing parameter order
    struct Child {
      explicit Child(storage::trie::RootHash const &hash) : hash{hash} {}
      storage::trie::RootHash const &hash;
    };

    /**
     * Mark \param child as the child trie of \param parent for pruning
     * purposes.
     */
    virtual outcome::result<void> markAsChild(Parent parent,
                                              common::Buffer const &key,
                                              Child child) = 0;

    /**
     * Prune the trie of a finalized block \param state.
     * Nodes belonging to this trie are deleted if no other trie references
     * them.
     * @param state header of the block which state should be pruned.
     * @param next_block the next finalized block after the pruned one
     * (required for pruner state persistency purposes).
     */
    virtual outcome::result<void> pruneFinalized(
        primitives::BlockHeader const &state) = 0;

    /**
     * Prune the trie of a discarded block \param state.
     * Nodes belonging to this trie are deleted if no other trie references
     * them.
     * @param state header of the block which state should be pruned.
     * (required for pruner state persistency purposes).
     */
    virtual outcome::result<void> pruneDiscarded(
        primitives::BlockHeader const &state) = 0;

    /**
     * @return the last pruned block.
     */
    virtual std::optional<primitives::BlockNumber> getLastPrunedBlock()
        const = 0;

    /**
     * @return the number of blocks behind the last finalized one
     * which states should be kept.
     */
    virtual std::optional<uint32_t> getPruningDepth() const = 0;
  };

}  // namespace kagome::storage::trie_pruner

#endif  // KAGOME_TRIEPRUNER_HPP
