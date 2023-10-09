/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "transaction_pool/impl/pool_moderator_impl.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "mock/core/clock/clock_mock.hpp"
#include "primitives/transaction.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using kagome::clock::SystemClock;
using kagome::clock::SystemClockMock;
using kagome::primitives::Transaction;
using kagome::transaction_pool::PoolModerator;
using kagome::transaction_pool::PoolModeratorImpl;
using testing::Return;

using namespace std::chrono_literals;

class PoolModeratorTest : public testing::Test {};

/**
 * @given a pool moderator
 * @when ban a transaction
 * @then when the transaction ban time ends and the pool is updated, the
 * transaction is no longer banned
 */
TEST_F(PoolModeratorTest, BanDurationCorrect) {
  auto clock = std::make_shared<SystemClockMock>();
  auto duration = 42min;
  SystemClockMock::TimePoint submit_time{10min};
  PoolModeratorImpl moderator(clock, {duration});
  testing::Expectation exp1 =
      EXPECT_CALL(*clock, now()).WillOnce(Return(submit_time));
  testing::Expectation exp2 = EXPECT_CALL(*clock, now())
                                  .After(exp1)
                                  .WillOnce(Return(submit_time + 20min));
  EXPECT_CALL(*clock, now())
      .After(exp2)
      .WillOnce(Return(submit_time + duration + 1min));
  Transaction t;
  t.hash = "beef"_hash256;
  moderator.ban(t.hash);
  ASSERT_TRUE(moderator.isBanned(t.hash));
  moderator.updateBan();
  ASSERT_FALSE(moderator.isBanned(t.hash));
}

/**
 * @given a pool moderator
 * @when banning a transaction if it's stale
 * @then a transaction is banned if it's stale and vice versa
 */
TEST_F(PoolModeratorTest, BanStaleCorrect) {
  auto clock = std::make_shared<SystemClockMock>();

  // timer invoked only in case when it is stale
  EXPECT_CALL(*clock, now()).WillOnce(Return(SystemClock::TimePoint{}));

  PoolModeratorImpl moderator(clock, {30min});
  Transaction t;
  t.valid_till = 42;
  t.hash = "abcd"_hash256;
  ASSERT_TRUE(moderator.banIfStale(43, t));
  Transaction t1;
  t1.valid_till = 42;
  t1.hash = "efef"_hash256;
  ASSERT_FALSE(moderator.banIfStale(41, t1));
}

/**
 * @given a pool moderator with expected size 5
 * @when the amount of banned trasaction reaches the limit of expected size * 2
 * @then the number of banned transactions drops to expected size 5
 */
TEST_F(PoolModeratorTest, UnbanWhenFull) {
  auto clock = std::make_shared<SystemClockMock>();
  PoolModeratorImpl moderator(clock, {1min, 5});

  constexpr size_t number_of_bans = 11;

  EXPECT_CALL(*clock, now())
      .Times(number_of_bans)
      .WillRepeatedly(Return(SystemClock::TimePoint{}));

  for (size_t i = 0; i < number_of_bans; i++) {
    kagome::common::Hash256 hash;
    std::fill(hash.begin(), hash.end(), 0);
    hash[0] = i;
    moderator.ban(hash);
  }
  // 11th ban exceeds the limit of EXPECTED_SIZE * 2 (5 * 2 = 10) and the number
  // of banned transactions drops to 5
  ASSERT_EQ(moderator.bannedNum(), 5);
}
