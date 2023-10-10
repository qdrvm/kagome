/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>
#include <unordered_set>

#include "consensus/grandpa/common.hpp"
#include "consensus/timeline/types.hpp"
#include "primitives/block_id.hpp"
#include "primitives/justification.hpp"

namespace kagome::blockchain {
  using BlockWeight = std::pair<uint32_t, primitives::BlockNumber>;

  /**
   * Used to update hashes of best chain by number.
   */
  struct Reorg {
    primitives::BlockInfo common;
    std::vector<primitives::BlockInfo> revert;
    std::vector<primitives::BlockInfo> apply;

    bool empty() const;
  };

  /**
   * Used to enqueue blocks for removal.
   * Children are removed before parent.
   */
  struct ReorgAndPrune {
    std::optional<Reorg> reorg;
    std::vector<primitives::BlockInfo> prune;
  };

  /**
   * In-memory light representation of the tree, used for efficiency and usage
   * convenience - we would only ask the database for some info, when directly
   * requested
   */
  class TreeNode {
   public:
    TreeNode(const primitives::BlockInfo &info);
    TreeNode(const primitives::BlockInfo &info,
             const std::shared_ptr<TreeNode> &parent,
             bool babe_primary);

    primitives::BlockInfo info;
    std::weak_ptr<TreeNode> weak_parent;
    uint32_t babe_primary_weight;
    bool contains_approved_para_block;
    bool reverted;

    std::vector<std::shared_ptr<TreeNode>> children{};

    std::shared_ptr<TreeNode> parent() const;
    BlockWeight weight() const;
  };

  Reorg reorg(std::shared_ptr<TreeNode> from, std::shared_ptr<TreeNode> to);

  bool canDescend(std::shared_ptr<TreeNode> from,
                  const std::shared_ptr<TreeNode> &to);

  /**
   * Non-finalized part of block tree
   */
  class CachedTree {
   public:
    explicit CachedTree(const primitives::BlockInfo &root);

    const std::shared_ptr<TreeNode> &finalized() const;
    const std::shared_ptr<TreeNode> &best() const;
    size_t leafCount() const;
    std::vector<primitives::BlockHash> leafHashes() const;
    bool isLeaf(const primitives::BlockHash &hash) const;
    primitives::BlockInfo bestWith(
        const std::shared_ptr<TreeNode> &required) const;
    std::shared_ptr<TreeNode> find(const primitives::BlockHash &hash) const;
    /**
     * Used when switching from fast-sync to full-sync.
     */
    ReorgAndPrune removeUnfinalized();

    /**
     * Remove nodes in block tree from current tree_ to {\arg new_trie_root}.
     * Needed to avoid cascade shared_ptr destructor calls which break
     * the stack.
     * @return new tree root
     */
    void updateTreeRoot(std::shared_ptr<TreeNode> new_trie_root);

    void updateMeta(const std::shared_ptr<TreeNode> &new_node);

    /// Force find and update actual best block
    void forceRefreshBest();

    /**
     * @brief
     * A reversal of updateMeta - it's called upon block tree branch prunung to
     * remove pruned block from leaves list, update deepest node wptr etc.
     * @param node Removed node
     */
    void removeFromMeta(const std::shared_ptr<TreeNode> &node);

   private:
    /**
     * Compare node weight with best and replace if heavier.
     * @return true if heavier and replaced.
     */
    bool chooseBest(std::shared_ptr<TreeNode> node);

    std::shared_ptr<TreeNode> root_;
    std::shared_ptr<TreeNode> best_;
    std::unordered_set<primitives::BlockHash> leaves_;
  };
}  // namespace kagome::blockchain
