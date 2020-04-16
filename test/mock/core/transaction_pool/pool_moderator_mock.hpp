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

#ifndef KAGOME_TEST_MOCK_CORE_TRANSACTION_POOL_POOL_MODERATOR_MOCK_HPP
#define KAGOME_TEST_MOCK_CORE_TRANSACTION_POOL_POOL_MODERATOR_MOCK_HPP

#include "transaction_pool/pool_moderator.hpp"

namespace kagome::transaction_pool {

  class PoolModeratorMock : public PoolModerator {
   public:
    MOCK_METHOD1(ban, void(const common::Hash256 &));
    MOCK_METHOD2(banIfStale,
                 bool(primitives::BlockNumber current_block,
                      const primitives::Transaction &tx));
    MOCK_CONST_METHOD1(isBanned, bool(const common::Hash256 &tx_hash));
    MOCK_METHOD0(updateBan, void());
    MOCK_CONST_METHOD0(bannedNum, size_t());
  };

}  // namespace kagome::transaction_pool

#endif  // KAGOME_TEST_MOCK_CORE_TRANSACTION_POOL_POOL_MODERATOR_MOCK_HPP
