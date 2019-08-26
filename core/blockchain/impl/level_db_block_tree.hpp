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
#include "blockchain/block_header_repository.hpp"
#include "blockchain/block_tree.hpp"
#include "blockchain/impl/common.hpp"
#include "common/logger.hpp"
#include "crypto/hasher.hpp"

namespace kagome::blockchain {
  /**
   * Block tree, which is located in LevelDB
   */
  class LevelDbBlockTree : public BlockTree {
    /**
     * In-memory light representation of the tree, used for efficiency and usage
     * convenience - we would only ask the database for some info, when directly
     * requested
     */
    struct TreeNode : public std::enable_shared_from_this<TreeNode> {
      TreeNode(primitives::BlockHash hash,
               primitives::BlockNumber depth,
               const std::shared_ptr<TreeNode> &parent,
               bool finalized = false);

      primitives::BlockHash block_hash_;
      primitives::BlockNumber depth_;

      std::weak_ptr<TreeNode> parent_;

      bool finalized_;

      std::vector<std::shared_ptr<TreeNode>> children_{};

      /**
       * Get a node of the tree, containing block with the specified hash, if it
       * can be found
       */
      std::shared_ptr<TreeNode> getByHash(const primitives::BlockHash &hash);

      bool operator==(const TreeNode &other) const;
      bool operator!=(const TreeNode &other) const;
    };

    /**
     * Useful information about the tree & blocks it contains to make some of
     * the operations faster
     */
    struct TreeMeta {
      TreeMeta(std::unordered_set<primitives::BlockHash> leaves,
               TreeNode &deepest_leaf,
               TreeNode &last_finalized);

      std::unordered_set<primitives::BlockHash> leaves;
      std::reference_wrapper<TreeNode> deepest_leaf;

      std::reference_wrapper<TreeNode> last_finalized;
    };

   public:
    /**
     * Create an instance of block tree
     * @param header_repo - block headers repository
     * @param db for the tree to be put in
     * @param last_finalized_block - last finalized block, from which the tree
     * is going to grow
     * @param hasher - pointer to the hasher
     * @return ptr to the created instance or error
     */
    static outcome::result<std::unique_ptr<LevelDbBlockTree>> create(
        std::shared_ptr<BlockHeaderRepository> header_repo,
        PersistentBufferMap &db,
        const primitives::BlockId &last_finalized_block,
        std::shared_ptr<crypto::Hasher> hasher,
        common::Logger log = common::createLogger("LevelDBBlockTree"));

    ~LevelDbBlockTree() override = default;

    outcome::result<primitives::BlockBody> getBlockBody(
        const primitives::BlockId &block) const override;

    outcome::result<primitives::Justification> getBlockJustification(
        const primitives::BlockId &block) const override;

    outcome::result<void> addBlock(primitives::Block block) override;

    outcome::result<void> finalize(
        const primitives::BlockHash &block,
        const primitives::Justification &justification) override;

    BlockHashVecRes getChainByBlock(
        const primitives::BlockHash &block) override;

    BlockHashVecRes getChainByBlock(const primitives::BlockHash &block,
                                    bool ascending,
                                    uint64_t maximum) override;
    BlockHashVecRes getChainByBlocks(
        const primitives::BlockHash &top_block,
        const primitives::BlockHash &bottom_block) override;

    BlockHashVecRes longestPath() override;

    const primitives::BlockHash &deepestLeaf() const override;

    std::vector<primitives::BlockHash> getLeaves() const override;

    BlockHashVecRes getChildren(const primitives::BlockHash &block) override;

    primitives::BlockHash getLastFinalized() const override;

    void prune() override;

   private:
    /**
     * Private ctor, so that instances are created only through the factory
     * method
     */
    LevelDbBlockTree(std::shared_ptr<BlockHeaderRepository> header_repo,
                     PersistentBufferMap &db,
                     std::shared_ptr<TreeNode> tree,
                     std::shared_ptr<TreeMeta> meta,
                     std::shared_ptr<crypto::Hasher> hasher,
                     common::Logger log);

    std::shared_ptr<BlockHeaderRepository> header_repo_;
    PersistentBufferMap &db_;

    std::shared_ptr<TreeNode> tree_;
    std::shared_ptr<TreeMeta> tree_meta_;

    std::shared_ptr<crypto::Hasher> hasher_;
    common::Logger log_;
  };
}  // namespace kagome::blockchain

#endif  // KAGOME_LEVEL_DB_BLOCK_TREE_HPP
