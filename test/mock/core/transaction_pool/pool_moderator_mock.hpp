/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "transaction_pool/pool_moderator.hpp"

namespace kagome::transaction_pool {

  class PoolModeratorMock : public PoolModerator {
   public:
    MOCK_METHOD(void, ban, (const Transaction::Hash &), (override));

    MOCK_METHOD(bool,
                banIfStale,
                (primitives::BlockNumber current_block, const Transaction &tx),
                (override));

    MOCK_METHOD(bool,
                isBanned,
                (const Transaction::Hash &tx_hash),
                (const, override));

    MOCK_METHOD(void, updateBan, (), (override));

    MOCK_METHOD(size_t, bannedNum, (), (const, override));
  };

}  // namespace kagome::transaction_pool
