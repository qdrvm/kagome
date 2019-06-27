/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_POOL_MODERATOR_HPP
#define KAGOME_POOL_MODERATOR_HPP

#include <chrono>

#include "primitives/block_id.hpp"
#include "primitives/transaction.hpp"
#include "transaction_pool/clock.hpp"

namespace kagome::transaction_pool {

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
     * @param tx
     */
    virtual void ban(const primitives::Transaction &tx) = 0;

    /**
     * Bans a transaction \param tx if its longevity is past \param
     * current_block
     * @return true if the transaction has been banned, false otherwise
     */
    virtual bool banIfStale(primitives::BlockNumber current_block,
                            const primitives::Transaction &tx) = 0;

    /**
     * @return true if \param tx is banned, false otherwise
     */
    virtual bool isBanned(const primitives::Transaction &tx) const = 0;

    /**
     * Unbans transaction which ban time is exceeded
     */
    virtual void updateBan() = 0;
  };

}  // namespace kagome::transaction_pool

#endif  // KAGOME_POOL_MODERATOR_HPP
