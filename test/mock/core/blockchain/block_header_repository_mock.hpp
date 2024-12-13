/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "blockchain/block_header_repository.hpp"

#include <gmock/gmock.h>

namespace kagome::blockchain {

  class BlockHeaderRepositoryMock : public BlockHeaderRepository {
   public:
    MOCK_METHOD(outcome::result<primitives::BlockNumber>,
                getNumberByHash,
                (const primitives::BlockHash &),
                (const, override));

    MOCK_METHOD(outcome::result<common::Hash256>,
                getHashByNumber,
                (primitives::BlockNumber),
                (const, override));

    MOCK_METHOD(outcome::result<primitives::BlockHeader>,
                getBlockHeader,
                (const primitives::BlockHash &),
                (const, override));

    MOCK_METHOD(outcome::result<std::optional<primitives::BlockHeader>>,
                tryGetBlockHeader,
                (const primitives::BlockHash &),
                (const, override));

    MOCK_METHOD(outcome::result<primitives::BlockHash>,
                getHashById,
                (const primitives::BlockId &),
                (const));

    MOCK_METHOD(outcome::result<primitives::BlockNumber>,
                getNumberById,
                (const primitives::BlockId &),
                (const));
  };
}  // namespace kagome::blockchain
