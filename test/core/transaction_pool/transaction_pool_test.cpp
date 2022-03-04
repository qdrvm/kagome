/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "transaction_pool/impl/transaction_pool_impl.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "mock/core/blockchain/block_header_repository_mock.hpp"
#include "mock/core/transaction_pool/pool_moderator_mock.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/prepare_loggers.hpp"
#include "transaction_pool/transaction_pool_error.hpp"

using kagome::blockchain::BlockHeaderRepositoryMock;
using kagome::common::Buffer;
using kagome::common::Hash256;
using kagome::primitives::Transaction;
using kagome::primitives::events::ExtrinsicSubscriptionEngine;
using kagome::subscription::ExtrinsicEventKeyRepository;
using kagome::transaction_pool::PoolModerator;
using kagome::transaction_pool::PoolModeratorMock;
using kagome::transaction_pool::TransactionPoolError;
using kagome::transaction_pool::TransactionPoolImpl;

using testing::NiceMock;
using testing::Return;

using namespace std::chrono_literals;

class TransactionPoolTest : public testing::Test {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {
    auto moderator = std::make_unique<NiceMock<PoolModeratorMock>>();
    auto header_repo = std::make_unique<BlockHeaderRepositoryMock>();
    auto engine = std::make_unique<ExtrinsicSubscriptionEngine>();
    auto extrinsic_event_key_repo =
        std::make_unique<ExtrinsicEventKeyRepository>();

    pool_ = std::make_shared<TransactionPoolImpl>(
        std::move(moderator),
        std::move(header_repo),
        std::move(engine),
        std::move(extrinsic_event_key_repo),
        TransactionPoolImpl::Limits{3, 4});
  }

 protected:
  std::shared_ptr<TransactionPoolImpl> pool_;
};

Transaction makeTx(Transaction::Hash hash,
                   std::initializer_list<Transaction::Tag> provides,
                   std::initializer_list<Transaction::Tag> requires,
                   Transaction::Longevity valid_till = 10000) {
  Transaction tx;
  tx.hash = std::move(hash);
  tx.provides = std::vector(provides);
  tx.requires = std::vector(requires);
  tx.valid_till = valid_till;
  return tx;
}

outcome::result<void> submit(TransactionPoolImpl &pool,
                             std::vector<Transaction> txs) {
  for (auto &tx : txs) {
    OUTCOME_TRY(pool.submitOne(std::move(tx)));
  }
  return outcome::success();
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
                               makeTx("02"_hash256, {{2}}, {{1}}),
                               makeTx("03"_hash256, {{3}}, {{2}}),
                               makeTx("04"_hash256, {{4}}, {{3}}),
                               makeTx("05"_hash256, {{5}}, {{4}})};

  EXPECT_OUTCOME_TRUE_1(submit(*pool_.get(), {txs[0], txs[2]}));
  EXPECT_EQ(pool_->getStatus().waiting_num, 1);
  ASSERT_EQ(pool_->getStatus().ready_num, 1);

  EXPECT_OUTCOME_TRUE_1(submit(*pool_.get(), {txs[1]}));
  EXPECT_EQ(pool_->getStatus().waiting_num, 0);
  ASSERT_EQ(pool_->getStatus().ready_num, 3);

  EXPECT_OUTCOME_TRUE_1(submit(*pool_.get(), {txs[3]}));
  EXPECT_EQ(pool_->getStatus().waiting_num, 1);
  ASSERT_EQ(pool_->getStatus().ready_num, 3);

  // already imported
  {
    auto outcome = submit(*pool_.get(), {txs[0]});
    ASSERT_TRUE(outcome.has_error());
    EXPECT_EQ(outcome.error(), TransactionPoolError::TX_ALREADY_IMPORTED);
  }

  // pool is full
  {
    auto outcome = submit(*pool_.get(), {txs[4]});
    ASSERT_TRUE(outcome.has_error());
    EXPECT_EQ(outcome.error(), TransactionPoolError::POOL_IS_FULL);
  }
}

/**
 * @given a set of transactions and transaction pool
 * @when import transactions to the pool
 * @then the transactions are imported and the pool status updates accordingly
 * to resolution of transaction dependencies. As the provided set of
 * transactions includes all required tags, once all transactions are imported
 * they all must be ready
 */
TEST_F(TransactionPoolTest, CorrectRemoveTx) {
  std::vector<Transaction> txs{makeTx("01"_hash256, {{1}}, {}),
                               makeTx("02"_hash256, {{2}}, {{1}}),
                               makeTx("03"_hash256, {{3}}, {{2}})};

  EXPECT_OUTCOME_TRUE_1(submit(*pool_.get(), {txs[0], txs[2]}));
  EXPECT_EQ(pool_->getStatus().waiting_num, 1);
  ASSERT_EQ(pool_->getStatus().ready_num, 1);

  EXPECT_OUTCOME_TRUE_1(submit(*pool_.get(), {txs[1]}));
  EXPECT_EQ(pool_->getStatus().waiting_num, 0);
  ASSERT_EQ(pool_->getStatus().ready_num, 3);

  EXPECT_OUTCOME_TRUE_1(pool_->removeOne("02"_hash256));
  EXPECT_EQ(pool_->getStatus().waiting_num, 1);
  ASSERT_EQ(pool_->getStatus().ready_num, 1);

  // tx unexists in pool
  {
    auto outcome = pool_->removeOne("02"_hash256);
    ASSERT_TRUE(outcome.has_error());
    EXPECT_EQ(outcome.error(), TransactionPoolError::TX_NOT_FOUND);
  }
}
