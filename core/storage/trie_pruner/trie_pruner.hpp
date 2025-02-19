/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <vector>

#include "common/buffer.hpp"
#include "primitives/block_header.hpp"
#include "primitives/block_id.hpp"
#include "primitives/common.hpp"
#include "storage/trie/types.hpp"

namespace kagome::blockchain {
  class BlockTree;
}

namespace kagome::storage::trie {
  class PolkadotTrie;
}

namespace kagome::storage::trie_pruner {

  /**
   * @brief Indicates whether the block, which state is pruned, is discarded or
   * an old finalized block
   */
  enum class PruneReason : uint8_t {
    Discarded,
    Finalized,
  };

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
     * Schedule pruning the trie state of a block \param block_info.
     * Nodes belonging to this trie are deleted if no other trie references
     * them.
     * @param root the root of the trie to prune.
     * @param block_info hash and number of the block with its state to be
     * pruned.
     * @param reason whether block is pruned because its finalized or discarded.
     */
    virtual void schedulePrune(const trie::RootHash &root,
                               const primitives::BlockInfo &block_info,
                               PruneReason reason) = 0;

    /**
     * Resets the pruner state, collects info about node reference count
     * starting from the last finalized block
     */
    virtual outcome::result<void> recoverState(
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

    /**
     * Reload pruner after warp sync.
     */
    virtual void restoreStateAtFinalized(
        const blockchain::BlockTree &block_tree) = 0;
  };

}  // namespace kagome::storage::trie_pruner
