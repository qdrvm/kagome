/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "blockchain/block_tree_error.hpp"
#include "consensus/babe/impl/block_executor_impl.hpp"
#include "consensus/babe/impl/threshold_util.hpp"

#include "mock/core/blockchain/block_tree_mock.hpp"
#include "mock/core/consensus/authority/authority_update_observer_mock.hpp"
#include "mock/core/consensus/babe/babe_util_mock.hpp"
#include "mock/core/consensus/grandpa/environment_mock.hpp"
#include "mock/core/consensus/validation/block_validator_mock.hpp"
#include "mock/core/crypto/hasher_mock.hpp"
#include "mock/core/runtime/core_mock.hpp"
#include "mock/core/runtime/offchain_worker_api_mock.hpp"
#include "mock/core/transaction_pool/transaction_pool_mock.hpp"

#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/prepare_loggers.hpp"

using kagome::authority::AuthorityUpdateObserver;
using kagome::authority::AuthorityUpdateObserverMock;
using kagome::blockchain::BlockTree;
using kagome::blockchain::BlockTreeError;
using kagome::blockchain::BlockTreeMock;
using kagome::common::Buffer;
using kagome::consensus::BabeBlockHeader;
using kagome::consensus::BabeUtil;
using kagome::consensus::BabeUtilMock;
using kagome::consensus::BlockExecutorImpl;
using kagome::consensus::BlockValidator;
using kagome::consensus::BlockValidatorMock;
using kagome::consensus::EpochDigest;
using kagome::consensus::grandpa::Environment;
using kagome::consensus::grandpa::EnvironmentMock;
using kagome::crypto::Hasher;
using kagome::crypto::HasherMock;
using kagome::crypto::VRFThreshold;
using kagome::primitives::Authority;
using kagome::primitives::AuthorityId;
using kagome::primitives::AuthorityList;
using kagome::primitives::BabeConfiguration;
using kagome::primitives::BlockData;
using kagome::primitives::BlockId;
using kagome::primitives::BlockInfo;
using kagome::primitives::BlockNumber;
using kagome::runtime::Core;
using kagome::runtime::CoreMock;
using kagome::runtime::OffchainWorkerApi;
using kagome::runtime::OffchainWorkerApiMock;
using kagome::transaction_pool::TransactionPool;
using kagome::transaction_pool::TransactionPoolMock;

using testing::_;

class BlockExecutorTest : public testing::Test {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {
    block_tree_ = std::make_shared<BlockTreeMock>();
    core_ = std::make_shared<CoreMock>();
    configuration_ = std::make_shared<BabeConfiguration>();
    block_validator_ = std::make_shared<BlockValidatorMock>();
    grandpa_environment_ = std::make_shared<EnvironmentMock>();
    tx_pool_ = std::make_shared<TransactionPoolMock>();
    hasher_ = std::make_shared<HasherMock>();
    authority_update_observer_ =
        std::make_shared<AuthorityUpdateObserverMock>();
    babe_util_ = std::make_shared<BabeUtilMock>();
    offchain_worker_api_ = std::make_shared<OffchainWorkerApiMock>();

    block_executor_ =
        std::make_shared<BlockExecutorImpl>(block_tree_,
                                            core_,
                                            configuration_,
                                            block_validator_,
                                            grandpa_environment_,
                                            tx_pool_,
                                            hasher_,
                                            authority_update_observer_,
                                            babe_util_,
                                            offchain_worker_api_);
  }

 protected:
  std::shared_ptr<BlockTreeMock> block_tree_;
  std::shared_ptr<CoreMock> core_;
  std::shared_ptr<BabeConfiguration> configuration_;
  std::shared_ptr<BlockValidatorMock> block_validator_;
  std::shared_ptr<EnvironmentMock> grandpa_environment_;
  std::shared_ptr<TransactionPoolMock> tx_pool_;
  std::shared_ptr<HasherMock> hasher_;
  std::shared_ptr<AuthorityUpdateObserverMock> authority_update_observer_;
  std::shared_ptr<BabeUtilMock> babe_util_;
  std::shared_ptr<OffchainWorkerApiMock> offchain_worker_api_;

  std::shared_ptr<BlockExecutorImpl> block_executor_;
};

/**
 * For correct work of authority set changes, digests should be processed
 * after a justification is applied, if one is present.
 * Otherwise, a situation may occur where digests think that the current block
 * is not finalized and execute the wrong logic.
 */
TEST_F(BlockExecutorTest, DigestsFollowJustification) {
  AuthorityList authorities{Authority{"auth0"_hash256, 1},
                            Authority{"auth1"_hash256, 1}};
  kagome::primitives::BlockHeader header{
      .parent_hash = "parent_hash"_hash256,
      .number = 42,
      .digest = kagome::primitives::Digest{
          kagome::primitives::PreRuntime{
              kagome::primitives::kBabeEngineId,
              Buffer{scale::encode(BabeBlockHeader{.authority_index = 1})
                         .value()}},
          kagome::primitives::Consensus{
              kagome::primitives::ScheduledChange{authorities, 0}},
          kagome::primitives::Seal{
              kagome::primitives::kBabeEngineId,
              Buffer{scale::encode(kagome::consensus::Seal{}).value()}}}};
  kagome::primitives::Justification justification{.data =
                                                      "justification_data"_buf};
  kagome::primitives::BlockData block_data{
      .hash = "some_block"_hash256,
      .header = header,
      .body = kagome::primitives::BlockBody{},
      .justification = justification};
  EXPECT_CALL(*block_tree_, getBlockBody(BlockId{"parent_hash"_hash256}))
      .WillOnce(testing::Return(kagome::primitives::BlockBody{}));
  EXPECT_CALL(*block_tree_, getBlockBody(BlockId{"some_hash"_hash256}))
      .WillOnce(testing::Return(BlockTreeError::NO_SUCH_BLOCK));
  EXPECT_CALL(*hasher_, blake2b_256(_))
      .WillOnce(testing::Return("some_hash"_hash256));
  EXPECT_CALL(*block_tree_, getEpochDescriptor(0, "parent_hash"_hash256))
      .WillOnce(testing::Return(
          EpochDigest{.authorities = {Authority{"auth2"_hash256, 1},
                                      Authority{"auth3"_hash256, 1}},
                      .randomness = "randomness"_hash256}));
  configuration_->leadership_rate.second = 42;
  EXPECT_CALL(
      *block_validator_,
      validateHeader(header,
                     0,
                     AuthorityId{"auth3"_hash256},
                     kagome::consensus::calculateThreshold(
                         configuration_->leadership_rate, authorities, 0),
                     "randomness"_hash256))
      .WillOnce(testing::Return(outcome::success()));
  EXPECT_CALL(*block_tree_, getBlockHeader(BlockId{"parent_hash"_hash256}))
      .WillOnce(testing::Return(kagome::primitives::BlockHeader{
          .parent_hash = "grandparent_hash"_hash256, .number = 40}));
  EXPECT_CALL(*block_tree_, getLastFinalized())
      .WillOnce(testing::Return(BlockInfo{40, "grandparent_hash"_hash256}))
      .WillOnce(testing::Return(BlockInfo{42, "some_hash"_hash256}));
  EXPECT_CALL(*block_tree_,
              getBestContaining("grandparent_hash"_hash256,
                                std::optional<BlockNumber>{}))
      .WillOnce(testing::Return(BlockInfo{41, "parent_hash"_hash256}));
  EXPECT_CALL(*core_, execute_block(_))
      .WillOnce(testing::Return(outcome::success()));
  EXPECT_CALL(*block_tree_, addBlock(_))
      .WillOnce(testing::Return(outcome::success()));

  {
    testing::InSequence s;
    EXPECT_CALL(
        *grandpa_environment_,
        applyJustification(BlockInfo{42, "some_hash"_hash256}, justification))
        .WillOnce(testing::Return(outcome::success()));
    EXPECT_CALL(*authority_update_observer_,
                onConsensus(_, BlockInfo{42, "some_hash"_hash256}, _))
        .WillOnce(testing::Return(outcome::success()));
  }
  EXPECT_CALL(
      *block_tree_,
      getBestContaining("some_hash"_hash256, std::optional<BlockNumber>{}))
      .WillOnce(testing::Return(BlockInfo{42, "some_hash"_hash256}));
  EXPECT_CALL(*offchain_worker_api_, offchain_worker(_, _))
      .WillOnce(testing::Return(outcome::success()));

  EXPECT_OUTCOME_TRUE_1(block_executor_->applyBlock(std::move(block_data)))
}
