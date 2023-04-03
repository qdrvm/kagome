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
    MOCK_METHOD(outcome::result<void>,
                setBlockTreeLeaves,
                (std::vector<primitives::BlockHash>),
                (override));

    MOCK_METHOD(outcome::result<std::vector<primitives::BlockHash>>,
                getBlockTreeLeaves,
                (),
                (const, override));

    MOCK_METHOD(outcome::result<primitives::BlockInfo>,
                getLastFinalized,
                (),
                (const, override));

    MOCK_METHOD(outcome::result<void>,
                assignNumberToHash,
                (const primitives::BlockInfo &),
                (override));

    MOCK_METHOD(outcome::result<void>,
                deassignNumberToHash,
                (primitives::BlockNumber),
                (override));

    MOCK_METHOD(outcome::result<std::optional<primitives::BlockHash>>,
                getBlockHash,
                (primitives::BlockNumber),
                (const, override));
    MOCK_METHOD(outcome::result<std::optional<primitives::BlockHash>>,
                getBlockHash,
                (const primitives::BlockId &),
                (const, override));

    MOCK_METHOD(outcome::result<bool>,
                hasBlockHeader,
                (const primitives::BlockHash &),
                (const, override));

    MOCK_METHOD(outcome::result<primitives::BlockHash>,
                putBlockHeader,
                (const primitives::BlockHeader &),
                (override));

    MOCK_METHOD(outcome::result<std::optional<primitives::BlockHeader>>,
                getBlockHeader,
                (const primitives::BlockHash &),
                (const, override));

    MOCK_METHOD(outcome::result<void>,
                putBlockBody,
                (const primitives::BlockHash &, const primitives::BlockBody &),
                (override));

    MOCK_METHOD(outcome::result<std::optional<primitives::BlockBody>>,
                getBlockBody,
                (const primitives::BlockHash &),
                (const, override));

    MOCK_METHOD(outcome::result<void>,
                removeBlockBody,
                (const primitives::BlockHash &),
                (override));

    MOCK_METHOD(outcome::result<void>,
                putJustification,
                (const primitives::Justification &,
                 const primitives::BlockHash &),
                (override));

    MOCK_METHOD(outcome::result<std::optional<primitives::Justification>>,
                getJustification,
                (const primitives::BlockHash &),
                (const, override));

    MOCK_METHOD(outcome::result<void>,
                removeJustification,
                (const primitives::BlockHash &),
                (override));

    MOCK_METHOD(outcome::result<primitives::BlockHash>,
                putBlock,
                (const primitives::Block &),
                (override));

    MOCK_METHOD(outcome::result<std::optional<primitives::BlockData>>,
                getBlockData,
                (const primitives::BlockHash &),
                (const, override));

    MOCK_METHOD(outcome::result<void>,
                removeBlock,
                (const primitives::BlockHash &),
                (override));
  };

}  // namespace kagome::blockchain

#endif  // KAGOME_BLOCK_STORAGE_MOCK_HPP
