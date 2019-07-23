/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/impl/block_tree_impl.hpp"

namespace kagome::blockchain {
  BlockTreeImpl::BlockTreeImpl(std::unique_ptr<storage::LevelDB> db)
      : db_{std::move(db)} {}

  outcome::result<primitives::Block> BlockTreeImpl::getBlock(
      const primitives::BlockId &block) const {

  }

  outcome::result<void> BlockTreeImpl::addBlock(
      const primitives::BlockId &parent, primitives::Block block) {}

  BlockTreeImpl::BlockHashResVec BlockTreeImpl::getChainByBlock(
      const primitives::BlockId &block) const {}

  BlockTreeImpl::BlockHashResVec BlockTreeImpl::longestPath(
      const primitives::BlockId &block) const {}

  const primitives::Block &BlockTreeImpl::deepestLeaf() const {}

  std::vector<primitives::BlockHash> BlockTreeImpl::getLeaves() const {}

  BlockTreeImpl::BlockHashResVec BlockTreeImpl::getChildren(
      const primitives::BlockId &block) const {}

  BlockTreeImpl::BlockHashRes BlockTreeImpl::getLastFinalized() const {}

  void BlockTreeImpl::prune() {}
}  // namespace kagome::blockchain
