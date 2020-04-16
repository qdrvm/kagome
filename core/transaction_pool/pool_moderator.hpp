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

#ifndef KAGOME_POOL_MODERATOR_HPP
#define KAGOME_POOL_MODERATOR_HPP

#include "primitives/block_id.hpp"
#include "primitives/transaction.hpp"

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
     * @param tx_hash
     */
    virtual void ban(const common::Hash256 &tx_hash) = 0;

    /**
     * Bans a transaction \param tx if its longevity is past \param
     * current_block
     * @return true if the transaction has been banned, false otherwise
     */
    virtual bool banIfStale(primitives::BlockNumber current_block,
                            const primitives::Transaction &tx) = 0;

    /**
     * @return true if \param tx_hash is banned, false otherwise
     */
    virtual bool isBanned(const common::Hash256 &tx_hash) const = 0;

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

#endif  // KAGOME_POOL_MODERATOR_HPP
