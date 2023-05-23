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

namespace kagome::blockchain {
  class BlockTree;
}

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
        const storage::trie::RootHash &state_root,
        trie::StateVersion version) = 0;

    /**
     * Register a new trie with the trie pruner so that the trie nodes
     * this trie references are kept until the block this trie belongs to is
     * pruned.
     * @note This overload avoids downloading trie nodes that are already in
     * memory from the database
     * @param version trie version used by runtime when creating this trie.
     */
    virtual outcome::result<void> addNewState(
        const trie::PolkadotTrie &new_trie, trie::StateVersion version) = 0;

    /**
     * Register a new child trie with the trie pruner so that the trie nodes
     * this trie references are kept until the block this trie belongs to is
     * pruned.
     * @param version trie version used by runtime when creating this trie.
     */
    virtual outcome::result<void> addNewChildState(
        const storage::trie::RootHash &parent_root,
        common::BufferView key,
        const trie::PolkadotTrie &new_trie,
        trie::StateVersion version) = 0;

    /// wrapper for RootHash to avoid confusing parameter order
    struct Parent {
      explicit Parent(const storage::trie::RootHash &hash) : hash{hash} {}
      const storage::trie::RootHash &hash;
    };

    /// wrapper for RootHash to avoid confusing parameter order
    struct Child {
      explicit Child(const storage::trie::RootHash &hash) : hash{hash} {}
      const storage::trie::RootHash &hash;
    };

    /**
     * Mark \param child as the child trie of \param parent for pruning
     * purposes.
     */
    virtual outcome::result<void> markAsChild(Parent parent,
                                              const common::Buffer &key,
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
        const primitives::BlockHeader &state) = 0;

    /**
     * Prune the trie of a discarded block \param state.
     * Nodes belonging to this trie are deleted if no other trie references
     * them.
     * @param state header of the block which state should be pruned.
     * (required for pruner state persistency purposes).
     */
    virtual outcome::result<void> pruneDiscarded(
        const primitives::BlockHeader &state) = 0;

    /** Resets the pruner state and builds it from scratch from the state
     * of the provided block
     */
    virtual outcome::result<void> restoreState(
        const primitives::BlockHeader &base_block,
        const blockchain::BlockTree &block_tree) = 0;

    /**
     * @return the last pruned block.
     */
    virtual std::optional<primitives::BlockInfo> getLastPrunedBlock() const = 0;

    /**
     * @return the number of blocks behind the last finalized one
     * which states should be kept.
     */
    virtual std::optional<uint32_t> getPruningDepth() const = 0;
  };

}  // namespace kagome::storage::trie_pruner

#endif  // KAGOME_TRIEPRUNER_HPP
