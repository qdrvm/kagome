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
    MOCK_METHOD(outcome::result<primitives::BlockBody>,
                getBlockBody,
                (const primitives::BlockId &),
                (const, override));

    MOCK_METHOD(outcome::result<primitives::BlockHeader>,
                getBlockHeader,
                (const primitives::BlockId &),
                (const, override));

    MOCK_METHOD(outcome::result<primitives::Justification>,
                getBlockJustification,
                (const primitives::BlockId &),
                (const, override));

    MOCK_METHOD(std::optional<primitives::Version>,
                runtimeVersion,
                (),
                (const, override));

    MOCK_METHOD(outcome::result<void>,
                addBlockHeader,
                (const primitives::BlockHeader &),
                (override));

    MOCK_METHOD(outcome::result<void>,
                addBlockBody,
                (primitives::BlockNumber block_number,
                 const primitives::BlockHash &block_hash,
                 const primitives::BlockBody &block_body),
                (override));

    MOCK_METHOD(outcome::result<void>,
                addExistingBlock,
                (const primitives::BlockHash &,
                 const primitives::BlockHeader &),
                (override));

    MOCK_METHOD(outcome::result<void>,
                addBlock,
                (const primitives::Block &),
                (override));

    MOCK_METHOD(outcome::result<void>,
                finalize,
                (const primitives::BlockHash &,
                 const primitives::Justification &),
                (override));

    MOCK_METHOD(BlockHashVecRes,
                getChainByBlock,
                (const primitives::BlockHash &),
                (const, override));

    MOCK_METHOD(BlockHashVecRes,
                getChainByBlock,
                (const primitives::BlockHash &, GetChainDirection, uint64_t),
                (const, override));

    MOCK_METHOD(BlockHashVecRes,
                getChainByBlocks,
                (const primitives::BlockHash &, const primitives::BlockHash &),
                (const, override));

    MOCK_METHOD(BlockHashVecRes,
                getChainByBlocks,
                (const primitives::BlockHash &,
                 const primitives::BlockHash &,
                 const uint32_t),
                (const, override));

    MOCK_METHOD(bool,
                hasDirectChain,
                (const primitives::BlockHash &, const primitives::BlockHash &),
                (const, override));

    MOCK_METHOD(outcome::result<primitives::BlockInfo>,
                getBestContaining,
                (const primitives::BlockHash &,
                 const std::optional<primitives::BlockNumber> &),
                (const, override));

    MOCK_METHOD(BlockHashVecRes, longestPath, (), (const, override));

    MOCK_METHOD(primitives::BlockInfo, deepestLeaf, (), (const, override));

    MOCK_METHOD(std::vector<primitives::BlockHash>,
                getLeaves,
                (),
                (const, override));

    MOCK_METHOD(BlockHashVecRes,
                getChildren,
                (const primitives::BlockHash &),
                (const, override));

    MOCK_METHOD(primitives::BlockInfo, getLastFinalized, (), (const, override));

    MOCK_METHOD(outcome::result<void>, prune, (), ());

    MOCK_METHOD(outcome::result<consensus::EpochDigest>,
                getEpochDescriptor,
                (consensus::EpochNumber, primitives::BlockHash),
                (const, override));
  };
}  // namespace kagome::blockchain

#endif  // KAGOME_BLOCK_TREE_MOCK_HPP
