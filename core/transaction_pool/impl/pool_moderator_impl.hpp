/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

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
                    const Transaction &tx) override;

    bool isBanned(const common::Hash256 &tx_hash) const override;

    void updateBan() override;

    size_t bannedNum() const override;

   private:
    std::shared_ptr<clock::SystemClock> clock_;
    Params params_;
    Map banned_until_;
  };

}  // namespace kagome::transaction_pool
