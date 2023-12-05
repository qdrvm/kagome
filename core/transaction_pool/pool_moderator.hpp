/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/block_id.hpp"
#include "primitives/transaction.hpp"

namespace kagome::transaction_pool {

  using primitives::Transaction;

  /**
   * PoolModerator is responsible for banning transaction for a fixed amount of
   * time to prevent them from returning to the transaction pool when it is
   * undesirable
   */
  class PoolModerator {
   public:
    virtual ~PoolModerator() = default;

    /**
     * Bans a transaction for a fixed amount of time
     * @param tx_hash
     */
    virtual void ban(const common::Hash256 &tx_hash) = 0;

    /**
     * Bans a transaction \param tx if its longevity is past \param
     * current_block
     * @return true if the transaction has been banned, false otherwise
     */
    virtual bool banIfStale(primitives::BlockNumber current_block,
                            const Transaction &tx) = 0;

    /**
     * @return true if \param tx_hash is banned, false otherwise
     */
    virtual bool isBanned(const Transaction::Hash &tx_hash) const = 0;

    /**
     * Unbans transaction which ban time is exceeded
     */
    virtual void updateBan() = 0;

    /**
     * Return the number of currently banned transactions
     */
    virtual size_t bannedNum() const = 0;
  };

}  // namespace kagome::transaction_pool
