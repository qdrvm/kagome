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

#ifndef KAGOME_POOL_MODERATOR_IMPL_HPP
#define KAGOME_POOL_MODERATOR_IMPL_HPP

#include "transaction_pool/pool_moderator.hpp"

#include <map>

#include "clock/clock.hpp"

namespace kagome::transaction_pool {

  class PoolModeratorImpl : public PoolModerator {
    using Map = std::map<common::Hash256, clock::SystemClock::TimePoint>;

   public:
    /**
     * Default value of expected size parameter
     */
    static constexpr size_t kDefaultExpectedSize = 2048;

    /**
     * Default ban duration
     */
    static constexpr clock::SystemClock::Duration kDefaultBanFor =
        std::chrono::minutes(30);

    /**
     * @param ban_for amount of time for which a transaction is banned
     * @param expected_size expected maximum number of banned transactions. If
     * significantly exceeded, some transactions will be removed from ban list
     */
    struct Params {
      clock::SystemClock::Duration ban_for = kDefaultBanFor;
      size_t expected_size = kDefaultExpectedSize;
    };

    /**
     * @param parameters configuration of the pool moderator
     * @param clock a clock used to determine when it is time to unban a
     * transaction
     */
    PoolModeratorImpl(std::shared_ptr<clock::SystemClock> clock,
                               Params parameters);

    ~PoolModeratorImpl() override = default;

    void ban(const common::Hash256 &tx_hash) override;

    bool banIfStale(primitives::BlockNumber current_block,
                    const primitives::Transaction &tx) override;

    bool isBanned(const common::Hash256 &tx_hash) const override;

    void updateBan() override;

    size_t bannedNum() const override;

   private:
    std::shared_ptr<clock::SystemClock> clock_;
    Params params_;
    Map banned_until_;
  };

}  // namespace kagome::transaction_pool

#endif  // KAGOME_POOL_MODERATOR_IMPL_HPP
