/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_BLOCKCHAIN_HEADER_BACKEND_MOCK_HPP
#define KAGOME_TEST_MOCK_BLOCKCHAIN_HEADER_BACKEND_MOCK_HPP

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

    MOCK_METHOD(outcome::result<kagome::blockchain::BlockStatus>,
                getBlockStatus,
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

#endif  // KAGOME_TEST_MOCK_BLOCKCHAIN_HEADER_BACKEND_MOCK_HPP
