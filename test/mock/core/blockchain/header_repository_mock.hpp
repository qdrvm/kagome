/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
