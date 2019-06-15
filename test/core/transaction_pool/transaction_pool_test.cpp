/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "transaction_pool/impl/transaction_pool_impl.hpp"

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

class MockClock : public Clock {
  MOCK_CONST_METHOD0(now, Clock::TimePoint());
};

TEST_F(TransactionPoolTest, BanDurationCorrect) {
  auto clock = std::shared_ptr<MockClock>();
  PoolModeratorImpl moderator(clock, std::chrono::minutes(42));
  

}
