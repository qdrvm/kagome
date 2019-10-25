/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BLOCK_TREE_MOCK_HPP
#define KAGOME_BLOCK_TREE_MOCK_HPP

#include "blockchain/block_tree.hpp"

#include <gmock/gmock.h>

namespace kagome::blockchain {
  struct BlockTreeMock : public BlockTree {
    MOCK_CONST_METHOD1(
        getBlockBody,
        outcome::result<primitives::BlockBody>(const primitives::BlockId &));

    MOCK_CONST_METHOD1(getBlockJustification,
                       outcome::result<primitives::Justification>(
                           const primitives::BlockId &));

    MOCK_METHOD1(addBlock, outcome::result<void>(primitives::Block));

    MOCK_METHOD2(finalize,
                 outcome::result<void>(const primitives::BlockHash &,
                                       const primitives::Justification &));

    MOCK_METHOD1(getChainByBlock,
                 BlockHashVecRes(const primitives::BlockHash &));

    MOCK_METHOD3(getChainByBlock,
                 BlockHashVecRes(const primitives::BlockHash &,
                                 bool,
                                 uint64_t));

    MOCK_METHOD2(getChainByBlocks,
                 BlockHashVecRes(const primitives::BlockHash &,
                                 const primitives::BlockHash &));

    MOCK_CONST_METHOD2(getBestContaining,
                       outcome::result<primitives::BlockInfo>(
                           const primitives::BlockHash &,
                           const boost::optional<primitives::BlockNumber> &));

    MOCK_METHOD0(longestPath, BlockHashVecRes());

    MOCK_CONST_METHOD0(deepestLeaf, primitives::BlockInfo());

    MOCK_CONST_METHOD0(getLeaves, std::vector<primitives::BlockHash>());

    MOCK_METHOD1(getChildren, BlockHashVecRes(const primitives::BlockHash &));

    MOCK_CONST_METHOD0(getLastFinalized, primitives::BlockInfo());

    MOCK_METHOD0(prune, outcome::result<void>());
  };
}  // namespace kagome::blockchain

#endif  // KAGOME_BLOCK_TREE_MOCK_HPP
