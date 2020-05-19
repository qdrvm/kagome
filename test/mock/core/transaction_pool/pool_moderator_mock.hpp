/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_CORE_TRANSACTION_POOL_POOL_MODERATOR_MOCK_HPP
#define KAGOME_TEST_MOCK_CORE_TRANSACTION_POOL_POOL_MODERATOR_MOCK_HPP

#include "transaction_pool/pool_moderator.hpp"

namespace kagome::transaction_pool {

  class PoolModeratorMock : public PoolModerator {
   public:
    MOCK_METHOD1(ban, void(const Transaction::Hash &));
    MOCK_METHOD2(banIfStale,
                 bool(primitives::BlockNumber current_block,
                      const Transaction &tx));
    MOCK_CONST_METHOD1(isBanned, bool(const Transaction::Hash &tx_hash));
    MOCK_METHOD0(updateBan, void());
    MOCK_CONST_METHOD0(bannedNum, size_t());
  };

}  // namespace kagome::transaction_pool

#endif  // KAGOME_TEST_MOCK_CORE_TRANSACTION_POOL_POOL_MODERATOR_MOCK_HPP
