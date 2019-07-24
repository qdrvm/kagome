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
      std::vector<TreeNode> children{};

      bool finalized = false;

      outcome::result<std::reference_wrapper<TreeNode>> getByHash(
          const primitives::BlockHash &hash) const;
    };

    /**
     * Useful information about the tree & blocks it contains to make some of
     * the operations faster
     */
    struct TreeMeta {
      std::unordered_set<TreeNode &> leaves;
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
     * @param genesis_block (root) of the tree
     * @return ptr to the created instance or error
     *
     * @note if genesis block is provided, the caller MUST ensure that the DB is
     * empty, so that clashes will not happen; if the genesis block is not
     * provided, the DB MUST contain it least one block, which is considered to
     * be the genesis one
     */
    static outcome::result<std::unique_ptr<LevelDbBlockTree>> create(
        PersistentBufferMap &db,
        boost::optional<primitives::Block> genesis_block);

    ~LevelDbBlockTree() override = default;

    outcome::result<primitives::BlockBody> getBlockBody(
        const primitives::BlockId &block) const override;

    outcome::result<void> addBlock(primitives::Block block) override;

    outcome::result<void> finalizeBlock(
        const primitives::BlockHash &block,
        const primitives::Justification &justification) override;

    BlockHashVecRes getChainByBlock(
        const primitives::BlockHash &block) const override;

    BlockHashVecRes longestPath(
        const primitives::BlockHash &block) const override;

    const primitives::BlockHash &deepestLeaf() const override;

    std::vector<primitives::BlockHash> getLeaves() const override;

    BlockHashVecRes getChildren(
        const primitives::BlockHash &block) const override;

    primitives::BlockHash getLastFinalized() const override;

    void prune() override;

   private:
    PersistentBufferMap &db_;

    TreeNode tree_;
    TreeMeta tree_meta_;
  };
}  // namespace kagome::blockchain

OUTCOME_HPP_DECLARE_ERROR(kagome::blockchain, LevelDbBlockTree::Error)

#endif  // KAGOME_LEVEL_DB_BLOCK_TREE_HPP
