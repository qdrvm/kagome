/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_LEVEL_DB_BLOCK_TREE_HPP
#define KAGOME_LEVEL_DB_BLOCK_TREE_HPP

#include <functional>
#include <memory>
#include <queue>
#include <unordered_set>

#include <boost/optional.hpp>
#include "blockchain/block_tree.hpp"
#include "blockchain/impl/common.hpp"

namespace kagome::blockchain {
  /**
   * Block tree, which is located in LevelDB
   */
  class LevelDbBlockTree : public BlockTree {
    /**
     * The tree itself
     */
    struct TreeNode {
      primitives::BlockHash block_hash;
      primitives::BlockNumber depth;

      boost::optional<TreeNode &> parent;

      bool finalized = false;

      std::vector<TreeNode> children{};

      outcome::result<std::reference_wrapper<TreeNode>> getByHash(
          const primitives::BlockHash &hash);

      bool operator==(const TreeNode &other) const;
      bool operator!=(const TreeNode &other) const;
    };

    /**
     * Useful information about the tree & blocks it contains to make some of
     * the operations faster
     */
    struct TreeMeta {
      std::unordered_set<TreeNode *> leaves;
      TreeNode &deepest_leaf;

      TreeNode &last_finalized;
    };

   public:
    enum class Error {
      INVALID_DB = 1,
      HASH_FAILED,
      NO_SUCH_BLOCK,
      INTERNAL_ERROR
    };

    /**
     * Create an instance of block tree
     * @param db for the tree to be put in
     * @param last_finalized_block - last finalized block, from which the tree
     * is going to grow
     * @return ptr to the created instance or error
     */
    static outcome::result<std::unique_ptr<LevelDbBlockTree>> create(
        PersistentBufferMap &db,
        const primitives::BlockId &last_finalized_block);

    ~LevelDbBlockTree() override = default;

    outcome::result<primitives::BlockBody> getBlockBody(
        const primitives::BlockId &block) const override;

    outcome::result<void> addBlock(primitives::Block block) override;

    outcome::result<void> finalizeBlock(
        const primitives::BlockHash &block,
        const primitives::Justification &justification) override;

    BlockHashVecRes getChainByBlock(
        const primitives::BlockHash &block) override;

    BlockHashVecRes longestPath(const primitives::BlockHash &block) override;

    const primitives::BlockHash &deepestLeaf() const override;

    std::vector<primitives::BlockHash> getLeaves() const override;

    BlockHashVecRes getChildren(const primitives::BlockHash &block) override;

    primitives::BlockHash getLastFinalized() const override;

    void prune() override;

   private:
    LevelDbBlockTree(PersistentBufferMap &db, TreeNode tree, TreeMeta meta);

    static outcome::result<common::Hash256> blockHash(
        const primitives::Block &block);

    PersistentBufferMap &db_;

    TreeNode tree_;
    TreeMeta tree_meta_;
  };
}  // namespace kagome::blockchain

OUTCOME_HPP_DECLARE_ERROR(kagome::blockchain, LevelDbBlockTree::Error)

#endif  // KAGOME_LEVEL_DB_BLOCK_TREE_HPP
