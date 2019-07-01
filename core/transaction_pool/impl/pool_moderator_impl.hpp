/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_POOL_MODERATOR_IMPL_HPP
#define KAGOME_POOL_MODERATOR_IMPL_HPP

#include <map>

#include "transaction_pool/impl/clock_impl.hpp"
#include "transaction_pool/pool_moderator.hpp"

namespace kagome::transaction_pool {

  class PoolModeratorImpl : public PoolModerator {
    static constexpr size_t kDefaultExpectedSize = 2048;

    using SystemClock = Clock<std::chrono::system_clock>;
    using Map = std::map<common::Hash256, SystemClock::TimePoint>;

   public:
    /**
     * @param ban_for amount of time for which a transaction is banned
     * @param clock a clock used to determine when it is time to unban a
     * transaction
     * @param expected_size expected maximum number of banned transactions. If
     * significantly exceeded, some transactions will be removed from ban list
     */
    explicit PoolModeratorImpl(
        SystemClock::Duration ban_for = std::chrono::minutes(30),
        std::shared_ptr<SystemClock> clock =
            std::make_shared<kagome::transaction_pool::SystemClock>(),
        size_t expected_size = kDefaultExpectedSize);

    ~PoolModeratorImpl() override = default;

    void ban(const common::Hash256 &tx_hash) override;

    bool banIfStale(primitives::BlockNumber current_block,
                    const primitives::Transaction &tx) override;

    bool isBanned(const common::Hash256 &tx_hash) const override;

    void updateBan() override;

    size_t bannedNum() const override;

   private:
    Map banned_until_;
    SystemClock::Duration ban_for_;
    std::shared_ptr<SystemClock> clock_;
    size_t expected_size_ = kDefaultExpectedSize;
  };

}  // namespace kagome::transaction_pool

#endif  // KAGOME_POOL_MODERATOR_IMPL_HPP
