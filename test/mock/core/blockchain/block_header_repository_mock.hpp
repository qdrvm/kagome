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
                (const common::Hash256 &hash),
                (const, override));

    MOCK_METHOD(outcome::result<common::Hash256>,
                getHashByNumber,
                (const primitives::BlockNumber &number),
                (const, override));

    MOCK_METHOD(outcome::result<primitives::BlockHeader>,
                getBlockHeader,
                (const primitives::BlockId &id),
                (const, override));

    MOCK_METHOD(outcome::result<kagome::blockchain::BlockStatus>,
                getBlockStatus,
                (const primitives::BlockId &id),
                (const, override));

    MOCK_METHOD(outcome::result<common::Hash256>,
                getHashById,
                (const primitives::BlockId &id),
                (const));

    MOCK_METHOD(outcome::result<primitives::BlockNumber>,
                getNumberById,
                (const primitives::BlockId &id),
                (const));
  };
}  // namespace kagome::blockchain

#endif  // KAGOME_TEST_MOCK_BLOCKCHAIN_HEADER_BACKEND_MOCK_HPP
