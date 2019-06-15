/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_POOL_MODERATOR_IMPL_HPP
#define KAGOME_POOL_MODERATOR_IMPL_HPP

#include <map>

#include "transaction_pool/pool_moderator.hpp"

namespace kagome::transaction_pool {

  class PoolModeratorImpl : public PoolModerator {
    static constexpr size_t kDefaultExpectedSize = 2048;

   public:
    /**
     * @param clock a clock used to determine when it is time to unban a
     * transaction
     * @param ban_for amount of time for which a transaction is banned
     * @param expected_size expected maximum number of banned transactions. If
     * significantly exceeded, some transactions will be removed from ban list
     */
    explicit PoolModeratorImpl(
        std::shared_ptr<Clock> clock,
        Clock::Duration ban_for = std::chrono::minutes(30),
        size_t expected_size = kDefaultExpectedSize);

    ~PoolModeratorImpl() override = default;

    void ban(const primitives::Transaction &tx) override;

    bool banIfStale(primitives::BlockNumber current_block,
                    const primitives::Transaction &tx) override;

    bool isBanned(const primitives::Transaction &tx) const override;

    void updateBan() override;

   private:
    static bool Compare(const common::Buffer &b1, const common::Buffer &b2);
    std::map<common::Buffer, Clock::TimePoint, decltype(&Compare)> banned_until_;
    Clock::Duration ban_for_;
    std::shared_ptr<Clock> clock_;
    size_t expected_size_ = kDefaultExpectedSize;
  };

}  // namespace kagome::transaction_pool

#endif  // KAGOME_POOL_MODERATOR_IMPL_HPP
