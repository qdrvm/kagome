/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "authorship/impl/proposer_impl.hpp"

#include <gtest/gtest.h>

#include "authorship/impl/block_builder_error.hpp"
#include "mock/core/authorship/block_builder_factory_mock.hpp"
#include "mock/core/authorship/block_builder_mock.hpp"
#include "mock/core/runtime/block_builder_api_mock.hpp"
#include "mock/core/transaction_pool/transaction_pool_mock.hpp"
#include "primitives/event_types.hpp"
#include "subscription/extrinsic_event_key_repository.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/prepare_loggers.hpp"

using ::testing::_;
using ::testing::Return;
using ::testing::Test;

using kagome::authorship::BlockBuilder;
using kagome::authorship::BlockBuilderError;
using kagome::authorship::BlockBuilderFactoryMock;
using kagome::authorship::BlockBuilderMock;
using kagome::authorship::ProposerImpl;
using kagome::common::Buffer;
using kagome::primitives::Block;
using kagome::primitives::BlockId;
using kagome::primitives::BlockNumber;
using kagome::primitives::Digest;
using kagome::primitives::Extrinsic;
using kagome::primitives::InherentData;
using kagome::primitives::InherentIdentifier;
using kagome::primitives::PreRuntime;
using kagome::primitives::Transaction;
using kagome::primitives::events::ExtrinsicSubscriptionEngine;
using kagome::runtime::BlockBuilderApiMock;
using kagome::subscription::ExtrinsicEventKeyRepository;
using kagome::transaction_pool::TransactionPoolMock;

// TODO (kamilsa): workaround unless we bump gtest version to 1.8.1+
namespace kagome::primitives {
  std::ostream &operator<<(std::ostream &s,
                           const detail::DigestItemCommon &dic) {
    return s;
  }
}  // namespace kagome::primitives

class ProposerTest : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  /**
   * Sets default behavior for BlockBuilderFactory and BlockBuilderApi:
   * BlockBuilderFactory creates BlockBuilderMock
   * BlockBuilerApi returns predefined InherentExtrinsics
   */
  void SetUp() override {
    ASSERT_TRUE(inherent_data_.putData(InherentIdentifier{}, Buffer{1, 2, 3}));

    block_builder_ = std::make_unique<BlockBuilderMock>();
    EXPECT_CALL(*block_builder_factory_,
                createProxy(expected_block_id_, inherent_digests_))
        .WillOnce(Return(block_builder_.get()));

    EXPECT_CALL(*block_builder_api_mock_,
                inherent_extrinsics(
                    kagome::primitives::BlockInfo{expected_number_ - 1,
                                                  "parent_block_hash"_hash256},
                    "storage_state"_hash256,
                    inherent_data_))
        .WillOnce(Return(inherent_xts));
  }

 protected:
  std::shared_ptr<BlockBuilderFactoryMock> block_builder_factory_ =
      std::make_shared<BlockBuilderFactoryMock>();
  std::shared_ptr<TransactionPoolMock> transaction_pool_ =
      std::make_shared<TransactionPoolMock>();
  std::shared_ptr<BlockBuilderApiMock> block_builder_api_mock_ =
      std::make_shared<BlockBuilderApiMock>();
  std::shared_ptr<ExtrinsicSubscriptionEngine> extrinsic_sub_engine_ =
      std::make_shared<ExtrinsicSubscriptionEngine>();
  std::shared_ptr<ExtrinsicEventKeyRepository> extrinsic_event_key_repo_ =
      std::make_shared<ExtrinsicEventKeyRepository>();

  std::unique_ptr<BlockBuilderMock> block_builder_;

  ProposerImpl proposer_{block_builder_factory_,
                         transaction_pool_,
                         block_builder_api_mock_,
                         extrinsic_sub_engine_,
                         extrinsic_event_key_repo_};

  BlockNumber expected_number_{42};
  BlockId expected_block_id_{expected_number_};

  Digest inherent_digests_{PreRuntime{}};

  InherentData inherent_data_;
  std::vector<Extrinsic> inherent_xts{Extrinsic{{3, 4, 5}}};
  Block expected_block{{}, {{{5, 4, 3}}}};
};

/**
 * @given BlockBuilderApi creating inherent extrinsics @and TransactionPool
 * returning extrinsics
 * @when Proposer created from these BlockBuilderApi and TransactionPool is
 * trying to create block @and deadline is bigger than current time
 * @then Block is created and it is equal to the block baked in BlockBuilder
 */
TEST_F(ProposerTest, CreateBlockSuccess) {
  // given
  // we push 2 xts: 1 from inherent_xts and one from transaction pool
  EXPECT_CALL(*block_builder_, pushExtrinsic(_))
      .WillOnce(Return(outcome::success()))
      .WillOnce(Return(outcome::success()));

  EXPECT_CALL(*block_builder_, getInherentExtrinsics(inherent_data_))
  .WillOnce(Return(inherent_xts));

  // getReadyTransaction will return vector with single transaction
  std::map<Transaction::Hash, std::shared_ptr<Transaction>> ready_transactions{
      std::make_pair("fakeHash"_hash256, std::make_shared<Transaction>())};

  EXPECT_CALL(*transaction_pool_, getReadyTransactions())
      .WillOnce(Return(ready_transactions));

  EXPECT_CALL(*transaction_pool_, removeOne("fakeHash"_hash256))
      .WillOnce(Return(outcome::success()));

  EXPECT_CALL(*transaction_pool_, removeStale(BlockId(expected_number_)))
      .WillOnce(Return(outcome::success()));

  EXPECT_CALL(*block_builder_, estimateBlockSize()).WillOnce(Return(1));

  EXPECT_CALL(*block_builder_, bake()).WillOnce(Return(expected_block));

  // when
  auto block_res =
      proposer_.propose(expected_number_, inherent_data_, inherent_digests_);

  // then
  ASSERT_TRUE(block_res);
  ASSERT_EQ(expected_block, block_res.value());
}

/**
 * @given BlockBuilderApi creating inherent extrinsics @and TransactionPool
 * returning extrinsics @and BlockBuilder that cannot accept extrinsics
 * @when Proposer created from these BlockBuilderApi and TransactionPool is
 * trying to create block
 * @then Block is not created
 */
TEST_F(ProposerTest, CreateBlockFailsWhenXtNotPushed) {
  // given
  EXPECT_CALL(*block_builder_, pushExtrinsic(inherent_xts[0]))
      .WillOnce(Return(outcome::failure(BlockBuilderError::BAD_MANDATORY)));

  // when
  auto block_res =
      proposer_.propose(expected_number_, inherent_data_, inherent_digests_);

  // then
  ASSERT_FALSE(block_res);
}

/**
 * @given BlockBuilderApi creating inherent extrinsics @and TransactionPool
 * returning extrinsics
 * @when Proposer created from these BlockBuilderApi and TransactionPool is
 * trying to create block @but push extrinsics to block builder failed with
 * DispatchError
 * @then Block is still created, because extrinsics failed with such error are
 * still included
 */
TEST_F(ProposerTest, PushFailed) {
  // given
  // we push 1 xt from inherent_xts and 1 Xt from transaction pool. Second push
  // fails
  EXPECT_CALL(*block_builder_, pushExtrinsic(_))
      .WillOnce(Return(outcome::success()))  // for inherent xt
      .WillOnce(Return(outcome::failure(
          boost::system::error_code{})));  // for xt from tx pool, will emit
                                           // Error: Success though
  EXPECT_CALL(*block_builder_, estimateBlockSize()).WillOnce(Return(1));
  EXPECT_CALL(*block_builder_, bake()).WillOnce(Return(expected_block));

  std::map<Transaction::Hash, std::shared_ptr<Transaction>> ready_transactions{
      std::make_pair("fakeHash"_hash256, std::make_shared<Transaction>())};

  EXPECT_CALL(*transaction_pool_, removeOne("fakeHash"_hash256))
      .WillOnce(Return(Transaction{}));
  EXPECT_CALL(*transaction_pool_, getReadyTransactions())
      .WillOnce(Return(ready_transactions));
  EXPECT_CALL(*transaction_pool_, removeStale(BlockId(expected_number_)))
      .WillOnce(Return(outcome::success()));

  // when
  auto block_res =
      proposer_.propose(expected_number_, inherent_data_, inherent_digests_);

  // then
  ASSERT_TRUE(block_res);
}
