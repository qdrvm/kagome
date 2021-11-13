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
    MOCK_METHOD(outcome::result<primitives::BlockHash>,
                getGenesisBlockHash,
                (),
                (const, override));

    MOCK_METHOD(outcome::result<primitives::BlockHash>,
                getLastFinalizedBlockHash,
                (),
                (const, override));

    MOCK_METHOD(outcome::result<void>,
                setLastFinalizedBlockHash,
                (const primitives::BlockHash &),
                (override));

    MOCK_METHOD(outcome::result<bool>,
                hasBlockHeader,
                (const primitives::BlockId &),
                (const, override));

    MOCK_METHOD(outcome::result<primitives::BlockHeader>,
                getBlockHeader,
                (const primitives::BlockId &),
                (const, override));

    MOCK_METHOD(outcome::result<primitives::BlockBody>,
                getBlockBody,
                (const primitives::BlockId &),
                (const, override));

    MOCK_METHOD(outcome::result<primitives::BlockData>,
                getBlockData,
                (const primitives::BlockId &id),
                (const, override));

    MOCK_METHOD(outcome::result<primitives::Justification>,
                getJustification,
                (const primitives::BlockId &),
                (const, override));

    MOCK_METHOD(outcome::result<primitives::BlockHash>,
                putBlockHeader,
                (const primitives::BlockHeader &header),
                (override));

    MOCK_METHOD(outcome::result<void>,
                putBlockData,
                (primitives::BlockNumber,
                 const primitives::BlockData &block_data),
                (override));

    MOCK_METHOD(outcome::result<primitives::BlockHash>,
                putBlock,
                (const primitives::Block &),
                (override));

    MOCK_METHOD(outcome::result<void>,
                putJustification,
                (const primitives::Justification &,
                 const primitives::BlockHash &,
                 const primitives::BlockNumber &),
                (override));

    MOCK_METHOD(outcome::result<void>,
                removeBlock,
                (const primitives::BlockHash &,
                 const primitives::BlockNumber &),
                (override));
  };

}  // namespace kagome::blockchain

#endif  // KAGOME_BLOCK_STORAGE_MOCK_HPP
