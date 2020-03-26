/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "transaction_pool/impl/transaction_pool_impl.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "mock/core/blockchain/header_repository_mock.hpp"
#include "mock/core/transaction_pool/pool_moderator_mock.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/storage/std_list_adapter.hpp"
#include "transaction_pool/impl/pool_moderator_impl.hpp"

using kagome::blockchain::HeaderRepositoryMock;
using kagome::common::Buffer;
using kagome::common::Hash256;
using kagome::face::ForwardIterator;
using kagome::face::GenericIterator;
using kagome::face::GenericList;
using kagome::primitives::Transaction;
using kagome::primitives::TransactionLongevity;
using kagome::primitives::TransactionTag;
using kagome::transaction_pool::PoolModerator;
using kagome::transaction_pool::PoolModeratorMock;
using kagome::transaction_pool::TransactionPoolImpl;

using test::StdListAdapter;
using testing::Return;
using testing::NiceMock;

using namespace std::chrono_literals;

class TransactionPoolTest : public testing::Test {
 public:
  void SetUp() override {
    auto moderator = std::make_unique<NiceMock<PoolModeratorMock>>();
    auto header_repo = std::make_unique<HeaderRepositoryMock>();
    pool_ = std::make_shared<TransactionPoolImpl>(std::move(moderator),
                                                  std::move(header_repo), TransactionPoolImpl::Limits{});
  }

 protected:
  std::shared_ptr<TransactionPoolImpl> pool_;
};

Transaction makeTx(Hash256 hash, std::initializer_list<TransactionTag> provides,
                   std::initializer_list<TransactionTag> requires,
                   TransactionLongevity valid_till = 10000) {
  Transaction tx;
  tx.hash = std::move(hash);
  tx.provides = std::vector(provides);
  tx.requires = std::vector(requires);
  tx.valid_till = valid_till;
  return tx;
}

/**
 * @given a set of transactions and transaction pool
 * @when import transactions to the pool
 * @then the transactions are imported and the pool status updates accordingly
 * to resolution of transaction dependencies. As the provided set of
 * transactions includes all required tags, once all transactions are imported
 * they all must be ready
 */
TEST_F(TransactionPoolTest, CorrectImportToReady) {
  std::vector<Transaction> txs{makeTx("01"_hash256, {{1}}, {}),
                               makeTx("02"_hash256, {{2}}, {}),
                               makeTx("03"_hash256, {{3}}, {{2}, {1}}),
                               makeTx("04"_hash256, {{4}}, {{5}}),
                               makeTx("05"_hash256, {{5}}, {{4}, {2}})};

  EXPECT_OUTCOME_TRUE_1(pool_->submit({txs[0], txs[2], txs[3], txs[4]}));
  EXPECT_EQ(pool_->getStatus().waiting_num, 2);
  ASSERT_EQ(pool_->getStatus().ready_num, 2);
  EXPECT_OUTCOME_TRUE_1(pool_->submit({txs[1]}));
  // already imported
  EXPECT_OUTCOME_FALSE_1(pool_->submit({txs[1]}));
  EXPECT_EQ(pool_->getStatus().waiting_num, 0);
  ASSERT_EQ(pool_->getStatus().ready_num, 5);
}

/**
 * @given a transaction pool and a set of transactions where one transaction
 * transitively depends on every other in the set
 * @when when pruning the tag of the aforementioned transaction
 * @then as by the time of pruning all transactions are ready, all of them are
 * pruned as ready dependencies of the pruned transaction
 */
TEST_F(TransactionPoolTest, PruneAllTags) {
  std::vector<Transaction> txs{makeTx("01"_hash256, {{1}}, {}),
                               makeTx("02"_hash256, {{2}}, {}),
                               makeTx("03"_hash256, {{3}}, {{2}, {1}}),
                               makeTx("04"_hash256, {{4}}, {{3}}),
                               makeTx("05"_hash256, {{5}}, {{4}, {2}})};

  EXPECT_OUTCOME_TRUE_1(
      pool_->submit({txs[0], txs[1], txs[2], txs[3], txs[4]}));

  auto pruned = pool_->pruneTag(42, "05"_unhex);
  ASSERT_TRUE(std::is_permutation(
      pruned.begin(), pruned.end(), txs.begin(), txs.end(),
      [](auto &tx1, auto &tx2) { return tx1.hash == tx2.hash; }));
  ASSERT_EQ(pool_->getStatus().ready_num, 0);
  ASSERT_EQ(pool_->getStatus().waiting_num, 0);
}

/**
 * @given a transaction pool with transactions
 * @when pruning a non-exisiting tag
 * @then nothing happens
 */
TEST_F(TransactionPoolTest, PruneWrongTags) {
  std::vector<Transaction> txs{
      makeTx("01"_hash256, {{1}}, {}), makeTx("02"_hash256, {{2}}, {{4}}),
      makeTx("03"_hash256, {{3}}, {{4}}),
      // 6 will be passed to pruneTag in the last argument
      makeTx("04"_hash256, {{4}}, {{3}, {6}}),
      makeTx("05"_hash256, {{5}}, {{4}, {3}})};

  EXPECT_OUTCOME_TRUE_1(
      pool_->submit({txs[0], txs[1], txs[2], txs[3], txs[4]}));

  // wrong tag 65
  auto pruned = pool_->pruneTag(42, {{6, 5}});
  ASSERT_EQ(pool_->getStatus().ready_num, 4);
  ASSERT_EQ(pool_->getStatus().waiting_num, 1);
  ASSERT_TRUE(pruned.empty());
}

/**
 * @given a transaction pool with transactions
 * @when prune a tag of a transaction
 * @then the transaction and all its dependencies are removed from ready queue
 */
TEST_F(TransactionPoolTest, PruneSomeTags) {
  std::vector<Transaction> txs{
      makeTx("01"_hash256, {{1}}, {}), makeTx("02"_hash256, {{2}}, {{4}}),
      makeTx("03"_hash256, {{3}}, {{4}}),
      // 6 will be passed to pruneTag in the last argument
      makeTx("04"_hash256, {{4}}, {{3}, {6}}),
      makeTx("05"_hash256, {{5}}, {{4}, {3}})};

  EXPECT_OUTCOME_TRUE_1(
      pool_->submit({txs[0], txs[1], txs[2], txs[3], txs[4]}));
  ASSERT_EQ(pool_->getStatus().ready_num, 4);
  ASSERT_EQ(pool_->getStatus().waiting_num, 1);

  auto pruned = pool_->pruneTag(42, {6});
  auto pruned_ = pool_->pruneTag(42, {5});
  pruned.insert(pruned.end(), pruned_.begin(), pruned_.end());
  ASSERT_EQ(pool_->getStatus().ready_num, 2);
  ASSERT_EQ(pool_->getStatus().waiting_num, 0);
  ASSERT_TRUE(std::is_permutation(
      pruned.begin(), pruned.end(), txs.begin() + 2, txs.end(),
      [](auto &tx1, auto &tx2) { return tx1.hash == tx2.hash; }));
}

TEST_F(TransactionPoolTest, PruneInSeveralSteps) {
  std::vector<Transaction> txs{
      makeTx("01"_hash256, {{1}}, {}), makeTx("02"_hash256, {{2}}, {{1}}),
      makeTx("03"_hash256, {{3}}, {{1}}), makeTx("04"_hash256, {{4}}, {{3}}),
      makeTx("05"_hash256, {{5}}, {{4}, {3}})};
  EXPECT_OUTCOME_TRUE_1(pool_->submit(txs));
  ASSERT_EQ(pool_->getStatus().ready_num, 5);
  pool_->pruneTag(42, {3});
  // 1 is not pruned despite it being a dependency of 3 because 2 also depends
  // on it
  ASSERT_EQ(pool_->getStatus().ready_num, 4);
  pool_->pruneTag(42, {2});
  ASSERT_EQ(pool_->getStatus().ready_num, 2);
  pool_->pruneTag(42, {5});
  ASSERT_EQ(pool_->getStatus().ready_num, 0);
}
