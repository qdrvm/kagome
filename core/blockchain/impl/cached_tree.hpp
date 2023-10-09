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
   * In-memory light representation of the tree, used for efficiency and usage
   * convenience - we would only ask the database for some info, when directly
   * requested
   */
  class TreeNode : public std::enable_shared_from_this<TreeNode> {
   public:
    TreeNode(const primitives::BlockInfo &info);
    TreeNode(const primitives::BlockInfo &info,
             const std::shared_ptr<TreeNode> &parent,
             bool babe_primary);

    primitives::BlockHash block_hash;
    primitives::BlockNumber depth;
    std::weak_ptr<TreeNode> parent;
    uint32_t babe_primary_weight;
    bool contains_approved_para_block;
    bool reverted;

    std::vector<std::shared_ptr<TreeNode>> children{};

    BlockWeight weight() const;

    /**
     * Get a node of the tree, containing block with the specified hash, if it
     * can be found
     */
    std::shared_ptr<const TreeNode> findByHash(
        const primitives::BlockHash &hash) const;

    std::shared_ptr<TreeNode> findByHash(const primitives::BlockHash &hash) {
      return std::const_pointer_cast<TreeNode>(
          std::as_const(*this).findByHash(hash));
    }

    primitives::BlockInfo getBlockInfo() const {
      return {depth, block_hash};
    }

    bool operator==(const TreeNode &other) const;
    bool operator!=(const TreeNode &other) const;
  };

  /**
   * Useful information about the tree & blocks it contains to make some of
   * the operations faster
   */
  struct TreeMeta {
    explicit TreeMeta(const std::shared_ptr<TreeNode> &subtree_root_node);

    /**
     * Compare node weight with best and replace if heavier.
     * @return true if heavier and replaced.
     */
    bool chooseBest(std::shared_ptr<TreeNode> node);

    /// Force find and update actual best block
    void forceRefreshBest();

    std::unordered_set<primitives::BlockHash> leaves;
    std::weak_ptr<TreeNode> best_block;

    std::weak_ptr<TreeNode> last_finalized;
  };

  /**
   * Non-finalized part of block tree
   */
  class CachedTree {
   public:
    explicit CachedTree(std::shared_ptr<TreeNode> root,
                        std::shared_ptr<TreeMeta> metadata)
        : root_{std::move(root)}, metadata_{std::move(metadata)} {
      BOOST_ASSERT(root_ != nullptr);
      BOOST_ASSERT(metadata_ != nullptr);
    }

    const std::shared_ptr<TreeNode> &finalized() const;
    const std::shared_ptr<TreeNode> &best() const;
    size_t leafCount() const;

    /**
     * Remove nodes in block tree from current tree_ to {\arg new_trie_root}.
     * Needed to avoid cascade shared_ptr destructor calls which break
     * the stack.
     * @return new tree root
     */
    void updateTreeRoot(std::shared_ptr<TreeNode> new_trie_root);

    void updateMeta(const std::shared_ptr<TreeNode> &new_node);

    void forceRefreshBest();

    /**
     * @brief
     * A reversal of updateMeta - it's called upon block tree branch prunung to
     * remove pruned block from leaves list, update deepest node wptr etc.
     * @param node Removed node
     */
    void removeFromMeta(const std::shared_ptr<TreeNode> &node);

    const TreeNode &getRoot() const;
    TreeNode &getRoot();

    const TreeMeta &getMetadata() const;

   private:
    std::shared_ptr<TreeNode> root_;
    std::shared_ptr<TreeNode> best_;
    std::shared_ptr<TreeMeta> metadata_;
  };
}  // namespace kagome::blockchain
