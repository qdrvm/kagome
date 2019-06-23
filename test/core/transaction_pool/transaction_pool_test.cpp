/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "transaction_pool/impl/transaction_pool_impl.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/storage/std_list_adapter.hpp"
#include "transaction_pool/impl/clock_impl.hpp"
#include "transaction_pool/impl/pool_moderator_impl.hpp"

using kagome::common::Buffer;
using kagome::face::ForwardIterator;
using kagome::face::GenericIterator;
using kagome::face::GenericList;
using kagome::primitives::Transaction;
using kagome::primitives::TransactionTag;
using kagome::transaction_pool::Clock;
using kagome::transaction_pool::PoolModerator;
using kagome::transaction_pool::PoolModeratorImpl;
using kagome::transaction_pool::SystemClock;
using kagome::transaction_pool::TransactionPool;
using kagome::transaction_pool::TransactionPoolImpl;
using test::StdListAdapter;
using testing::Return;
using namespace std::chrono_literals;

class TransactionPoolTest : public testing::Test {
 public:
  void SetUp() {
    auto clock_ = std::make_shared<SystemClock>();
    auto moderator_ = std::make_unique<PoolModeratorImpl>(clock_);
    pool_ = TransactionPoolImpl::create<StdListAdapter>(std::move(moderator_));
  }

  std::shared_ptr<TransactionPool> pool_;
};

Transaction makeTx(Buffer hash, std::initializer_list<TransactionTag> provides,
                   std::initializer_list<TransactionTag> requires) {
  Transaction tx;
  tx.hash = std::move(hash);
  tx.provides = std::vector(provides);
  tx.requires = std::vector(requires);
  return tx;
}

TEST_F(TransactionPoolTest, Create) {
  StdListAdapter<int> list;
  std::vector<int> v{1, 2, 3, 4, 5};
  std::copy(v.begin(), v.end(), std::back_inserter(list));
  std::stable_partition(list.begin(), list.end(),
                        [](int x) { return x % 2 == 0; });
}

TEST_F(TransactionPoolTest, CorrectImportToReady) {
  std::vector<Transaction> txs{
      makeTx("01"_hex2buf, {"01"_unhex}, {}),
      makeTx("02"_hex2buf, {"02"_unhex}, {}),
      makeTx("03"_hex2buf, {}, {"02"_unhex, "01"_unhex}),
      makeTx("04"_hex2buf, {"04"_unhex}, {"05"_unhex}),
      makeTx("05"_hex2buf, {"05"_unhex}, {"04"_unhex, "02"_unhex})};

  EXPECT_OUTCOME_TRUE_1(pool_->submit({txs[0], txs[2], txs[3], txs[4]}));
  EXPECT_EQ(pool_->getStatus().waiting_num, 2);
  ASSERT_EQ(pool_->getStatus().ready_num, 2);
  EXPECT_OUTCOME_TRUE_1(pool_->submit({txs[1]}));
  EXPECT_EQ(pool_->getStatus().waiting_num, 0);
  ASSERT_EQ(pool_->getStatus().ready_num, 5);
}

 class MockClock : public Clock<std::chrono::system_clock> {
 public:
  MOCK_CONST_METHOD0(now, MockClock::TimePoint(void));
};

TEST_F(TransactionPoolTest, BanDurationCorrect) {
  auto clock = std::make_shared<MockClock>();
  auto duration = 42min;
  MockClock::TimePoint submit_time {10min};
  PoolModeratorImpl moderator(clock, duration);
  testing::Expectation exp1 =
      EXPECT_CALL(*clock, now()).WillOnce(Return(submit_time));
  testing::Expectation exp2 = EXPECT_CALL(*clock, now())
                                  .After(exp1)
                                  .WillOnce(Return(submit_time + 20min));
  EXPECT_CALL(*clock, now())
      .After(exp2)
      .WillOnce(Return(submit_time + duration + 1min));
  Transaction t;
  t.hash = "beef"_hex2buf;
  moderator.ban(t);
  ASSERT_TRUE(moderator.isBanned(t));
  moderator.updateBan();
  ASSERT_FALSE(moderator.isBanned(t));
}

TEST_F(TransactionPoolTest, BanStaleCorrect) {
  auto clock = std::make_shared<SystemClock>();
  PoolModeratorImpl moderator(clock);
  Transaction t;
  t.valid_till = 42;
  t.hash = "abcd"_hex2buf;
  moderator.banIfStale(43, t);
  ASSERT_TRUE(moderator.isBanned(t));
  Transaction t1;
  t1.valid_till = 42;
  t1.hash = "efef"_hex2buf;
  moderator.banIfStale(41, t1);
  ASSERT_FALSE(moderator.isBanned(t1));
}
