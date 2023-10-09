/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "blockchain/block_tree.hpp"

#include <gmock/gmock.h>

namespace kagome::blockchain {
  struct BlockTreeMock : public BlockTree {
    MOCK_METHOD(const primitives::BlockHash &,
                getGenesisBlockHash,
                (),
                (const, override));

    MOCK_METHOD(outcome::result<std::optional<primitives::BlockHash>>,
                getBlockHash,
                (primitives::BlockNumber),
                (const, override));

    MOCK_METHOD(outcome::result<bool>,
                hasBlockHeader,
                (const primitives::BlockHash &),
                (const, override));

    MOCK_METHOD(outcome::result<primitives::BlockBody>,
                getBlockBody,
                (const primitives::BlockHash &),
                (const, override));

    MOCK_METHOD(outcome::result<primitives::BlockHeader>,
                getBlockHeader,
                (const primitives::BlockHash &),
                (const, override));

    MOCK_METHOD(outcome::result<primitives::Justification>,
                getBlockJustification,
                (const primitives::BlockHash &),
                (const, override));

    MOCK_METHOD(outcome::result<void>,
                addBlockHeader,
                (const primitives::BlockHeader &),
                (override));

    MOCK_METHOD(outcome::result<void>,
                addBlockBody,
                (const primitives::BlockHash &, const primitives::BlockBody &),
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
                markAsParachainDataBlock,
                (const primitives::BlockHash &),
                (override));

    MOCK_METHOD(outcome::result<void>,
                markAsRevertedBlocks,
                (const std::vector<primitives::BlockHash> &),
                (override));

    MOCK_METHOD(outcome::result<void>,
                removeLeaf,
                (const primitives::BlockHash &),
                (override));

    MOCK_METHOD(outcome::result<void>,
                finalize,
                (const primitives::BlockHash &,
                 const primitives::Justification &),
                (override));

    MOCK_METHOD(BlockHashVecRes,
                getBestChainFromBlock,
                (const primitives::BlockHash &, uint64_t),
                (const, override));

    MOCK_METHOD(BlockHashVecRes,
                getDescendingChainToBlock,
                (const primitives::BlockHash &, uint64_t),
                (const, override));

    MOCK_METHOD(BlockHashVecRes,
                getChainByBlocks,
                (const primitives::BlockHash &, const primitives::BlockHash &),
                (const, override));

    MOCK_METHOD(bool,
                hasDirectChain,
                (const primitives::BlockHash &, const primitives::BlockHash &),
                (const, override));

    MOCK_METHOD(bool,
                isFinalized,
                (const primitives::BlockInfo &),
                (const, override));

    MOCK_METHOD(outcome::result<primitives::BlockInfo>,
                getBestContaining,
                (const primitives::BlockHash &),
                (const, override));

    MOCK_METHOD(primitives::BlockInfo, bestBlock, (), (const, override));

    MOCK_METHOD(std::vector<primitives::BlockHash>,
                getLeaves,
                (),
                (const, override));

    MOCK_METHOD(BlockHashVecRes,
                getChildren,
                (const primitives::BlockHash &),
                (const, override));

    MOCK_METHOD(primitives::BlockInfo, getLastFinalized, (), (const, override));

    MOCK_METHOD(void, warp, (const primitives::BlockInfo &), (override));

    MOCK_METHOD(void, notifyBestAndFinalized, (), (override));
  };
}  // namespace kagome::blockchain
