/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_BLOCKCHAIN_HEADER_BACKEND_MOCK_HPP
#define KAGOME_TEST_MOCK_BLOCKCHAIN_HEADER_BACKEND_MOCK_HPP

#include "blockchain/block_header_repository.hpp"

#include <gmock/gmock.h>

namespace kagome::blockchain {

  class HeaderRepositoryMock : public BlockHeaderRepository {
   public:
    MOCK_CONST_METHOD1(getNumberByHash, outcome::result<primitives::BlockNumber> (
        const common::Hash256 &hash));
    MOCK_CONST_METHOD1(getHashByNumber, outcome::result<common::Hash256> (
        const primitives::BlockNumber &number));
    MOCK_CONST_METHOD1(getBlockHeader, outcome::result<primitives::BlockHeader> (
        const primitives::BlockId &id));
    MOCK_CONST_METHOD1(getBlockStatus, outcome::result<kagome::blockchain::BlockStatus> (
        const primitives::BlockId &id));
    MOCK_CONST_METHOD1(getHashById, outcome::result<common::Hash256> (
        const primitives::BlockId &id));
    MOCK_CONST_METHOD1(getNumberById, outcome::result<primitives::BlockNumber> (
        const primitives::BlockId &id));
  };
}  // namespace kagome::blockchain

#endif  // KAGOME_TEST_MOCK_BLOCKCHAIN_HEADER_BACKEND_MOCK_HPP
