/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>
#include <unordered_map>
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

    primitives::BlockInfo finalized() const;
    primitives::BlockInfo best() const;
    size_t leafCount() const;
    std::vector<primitives::BlockHash> leafHashes() const;
    bool isLeaf(const primitives::BlockHash &hash) const;
    primitives::BlockInfo bestWith(
        const std::shared_ptr<TreeNode> &required) const;
    std::shared_ptr<TreeNode> find(const primitives::BlockHash &hash) const;
    std::optional<Reorg> add(const std::shared_ptr<TreeNode> &new_node);
    ReorgAndPrune finalize(const std::shared_ptr<TreeNode> &new_finalized);
    /**
     * Can't remove finalized root.
     */
    ReorgAndPrune removeLeaf(const primitives::BlockHash &hash);
    /**
     * Used when switching from fast-sync to full-sync.
     */
    ReorgAndPrune removeUnfinalized();

    /// Force find and update actual best block
    void forceRefreshBest();

   private:
    /**
     * Compare node weight with best and replace if heavier.
     * @return true if heavier and replaced.
     */
    bool chooseBest(std::shared_ptr<TreeNode> node);

    std::shared_ptr<TreeNode> root_;
    std::shared_ptr<TreeNode> best_;
    std::unordered_map<primitives::BlockHash, std::shared_ptr<TreeNode>> nodes_;
    std::unordered_set<primitives::BlockHash> leaves_;
  };
}  // namespace kagome::blockchain
