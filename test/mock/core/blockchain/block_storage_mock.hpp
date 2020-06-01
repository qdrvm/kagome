/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BLOCK_STORAGE_MOCK_HPP
#define KAGOME_BLOCK_STORAGE_MOCK_HPP

#include <gmock/gmock.h>
#include "blockchain/block_storage.hpp"

namespace kagome::blockchain {

  class BlockStorageMock : public BlockStorage {
   public:
    MOCK_CONST_METHOD0(getLastFinalizedBlockHash,
                       outcome::result<primitives::BlockHash>());

    MOCK_METHOD1(setLastFinalizedBlockHash,
                 outcome::result<void>(const primitives::BlockHash &));

    MOCK_CONST_METHOD1(
        getBlockHeader,
        outcome::result<primitives::BlockHeader>(const primitives::BlockId &));

    MOCK_CONST_METHOD1(
        getBlockBody,
        outcome::result<primitives::BlockBody>(const primitives::BlockId &));

    MOCK_CONST_METHOD1(
        getBlockData,
        outcome::result<primitives::BlockData>(const primitives::BlockId &id));

    MOCK_CONST_METHOD1(getJustification,
                       outcome::result<primitives::Justification>(
                           const primitives::BlockId &));

    MOCK_METHOD1(putBlockHeader,
                 outcome::result<primitives::BlockHash>(
                     const primitives::BlockHeader &header));

    MOCK_METHOD2(
        putBlockData,
        outcome::result<void>(primitives::BlockNumber,
                              const primitives::BlockData &block_data));

    MOCK_METHOD1(
        putBlock,
        outcome::result<primitives::BlockHash>(const primitives::Block &));

    MOCK_METHOD3(putJustification,
                 outcome::result<void>(const primitives::Justification &,
                                       const primitives::BlockHash &,
                                       const primitives::BlockNumber &));

    MOCK_METHOD2(removeBlock,
                 outcome::result<void>(const primitives::BlockHash &,
                                       const primitives::BlockNumber &));
  };

}  // namespace kagome::blockchain

#endif  // KAGOME_BLOCK_STORAGE_MOCK_HPP
