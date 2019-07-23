/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BLOCK_TREE_IMPL_HPP
#define KAGOME_BLOCK_TREE_IMPL_HPP

#include <memory>

#include "blockchain/block_tree.hpp"
#include "storage/leveldb/leveldb.hpp"

namespace kagome::blockchain {
  class BlockTreeImpl : public BlockTree {
    struct TreeNode {
      primitives::BlockHash block_hash;
      std::vector<TreeNode> children;
    };

   public:
    explicit BlockTreeImpl(std::unique_ptr<storage::LevelDB> db);

    ~BlockTreeImpl() override = default;

    outcome::result<primitives::Block> getBlock(
        const primitives::BlockId &block) const override;

    outcome::result<void> addBlock(const primitives::BlockId &parent,
                                   primitives::Block block) override;

    BlockHashResVec getChainByBlock(
        const primitives::BlockId &block) const override;

    BlockHashResVec longestPath(
        const primitives::BlockId &block) const override;

    const primitives::Block &deepestLeaf() const override;

    std::vector<primitives::BlockHash> getLeaves() const override;

    BlockHashResVec getChildren(
        const primitives::BlockId &block) const override;

    BlockHashRes getLastFinalized() const override;

    void prune() override;

   private:
    /// database, in which the actual blocks are stored
    std::unique_ptr<storage::LevelDB> db_;

    /// tree representation of the blocks' hashes for efficient usage of
    /// algorithms
    TreeNode hash_tree_;
  };
}  // namespace kagome::blockchain

#endif  // KAGOME_BLOCK_TREE_IMPL_HPP
