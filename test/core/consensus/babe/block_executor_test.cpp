/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/babe/impl/block_executor_impl.hpp"

#include <gtest/gtest.h>
#include <iostream>

#include "blockchain/block_tree_error.hpp"
#include "consensus/babe/impl/block_appender_base.hpp"
#include "consensus/babe/impl/threshold_util.hpp"
#include "consensus/babe/types/seal.hpp"
#include "mock/core/blockchain/block_tree_mock.hpp"
#include "mock/core/blockchain/digest_tracker_mock.hpp"
#include "mock/core/consensus/babe/babe_config_repository_mock.hpp"
#include "mock/core/consensus/babe/babe_util_mock.hpp"
#include "mock/core/consensus/babe/consistency_keeper_mock.hpp"
#include "mock/core/consensus/grandpa/environment_mock.hpp"
#include "mock/core/consensus/grandpa/grandpa_digest_observer_mock.hpp"
#include "mock/core/consensus/validation/block_validator_mock.hpp"
#include "mock/core/crypto/hasher_mock.hpp"
#include "mock/core/runtime/core_mock.hpp"
#include "mock/core/runtime/offchain_worker_api_mock.hpp"
#include "mock/core/transaction_pool/transaction_pool_mock.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/prepare_loggers.hpp"

using kagome::blockchain::BlockTree;
using kagome::blockchain::BlockTreeError;
using kagome::blockchain::BlockTreeMock;
using kagome::blockchain::DigestTrackerMock;
using kagome::common::Buffer;
using kagome::consensus::babe::BabeBlockHeader;
using kagome::consensus::babe::BabeConfigRepositoryMock;
using kagome::consensus::babe::BabeUtil;
using kagome::consensus::babe::BabeUtilMock;
using kagome::consensus::babe::BlockAppenderBase;
using kagome::consensus::babe::BlockExecutorImpl;
using kagome::consensus::babe::BlockValidator;
using kagome::consensus::babe::BlockValidatorMock;
using kagome::consensus::babe::ConsistencyGuard;
using kagome::consensus::babe::ConsistencyKeeperMock;
using kagome::consensus::babe::EpochDigest;
using kagome::consensus::grandpa::Environment;
using kagome::consensus::grandpa::EnvironmentMock;
using kagome::consensus::grandpa::GrandpaDigestObserverMock;
using kagome::crypto::Hasher;
using kagome::crypto::HasherMock;
using kagome::crypto::VRFThreshold;
using kagome::primitives::Authority;
using kagome::primitives::AuthorityId;
using kagome::primitives::AuthorityList;
using kagome::primitives::BabeConfiguration;
using kagome::primitives::Block;
using kagome::primitives::BlockContext;
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

using namespace std::chrono_literals;

using testing::_;
using testing::Return;
using testing::ReturnRef;

// TODO (kamilsa): workaround unless we bump gtest version to 1.8.1+
namespace kagome::primitives {
  std::ostream &operator<<(std::ostream &s,
                           const detail::DigestItemCommon &dic) {
    return s;
  }

  std::ostream &operator<<(std::ostream &s, const ScheduledChange &) {
    return s;
  }

  std::ostream &operator<<(std::ostream &s, const ForcedChange &) {
    return s;
  }

  std::ostream &operator<<(std::ostream &s, const OnDisabled &) {
    return s;
  }

  std::ostream &operator<<(std::ostream &s, const Pause &) {
    return s;
  }

  std::ostream &operator<<(std::ostream &s, const Resume &) {
    return s;
  }
}  // namespace kagome::primitives

class BlockExecutorTest : public testing::Test {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {
    block_tree_ = std::make_shared<BlockTreeMock>();
    core_ = std::make_shared<CoreMock>();

    babe_config_ = std::make_shared<BabeConfiguration>();
    babe_config_->slot_duration = 60ms;
    babe_config_->epoch_length = 2;
    babe_config_->leadership_rate = {1, 4};
    babe_config_->authorities = {Authority{{"auth2"_hash256}, 1},
                                 Authority{{"auth3"_hash256}, 1}};
    babe_config_->randomness = "randomness"_hash256;

    babe_config_repo_ = std::make_shared<BabeConfigRepositoryMock>();
    ON_CALL(*babe_config_repo_, config(_, _))
        .WillByDefault(Return(std::make_optional(std::cref(*babe_config_))));

    block_validator_ = std::make_shared<BlockValidatorMock>();
    grandpa_environment_ = std::make_shared<EnvironmentMock>();
    tx_pool_ = std::make_shared<TransactionPoolMock>();
    hasher_ = std::make_shared<HasherMock>();
    digest_tracker_ = std::make_shared<DigestTrackerMock>();

    babe_util_ = std::make_shared<BabeUtilMock>();
    ON_CALL(*babe_util_, syncEpoch(_)).WillByDefault(Return(1));
    ON_CALL(*babe_util_, slotToEpoch(_)).WillByDefault(Return(1));

    offchain_worker_api_ = std::make_shared<OffchainWorkerApiMock>();
    storage_sub_engine_ = std::make_shared<
        kagome::primitives::events::StorageSubscriptionEngine>();
    chain_sub_engine_ =
        std::make_shared<kagome::primitives::events::ChainSubscriptionEngine>();
    consistency_keeper_ = std::make_shared<ConsistencyKeeperMock>();

    auto appender = std::make_unique<BlockAppenderBase>(consistency_keeper_,
                                                        block_tree_,
                                                        digest_tracker_,
                                                        babe_config_repo_,
                                                        block_validator_,
                                                        grandpa_environment_,
                                                        babe_util_,
                                                        hasher_);

    block_executor_ = std::make_shared<BlockExecutorImpl>(block_tree_,
                                                          core_,
                                                          tx_pool_,
                                                          hasher_,
                                                          offchain_worker_api_,
                                                          storage_sub_engine_,
                                                          chain_sub_engine_,
                                                          std::move(appender));
  }

 protected:
  std::shared_ptr<BlockTreeMock> block_tree_;
  std::shared_ptr<CoreMock> core_;
  std::shared_ptr<BabeConfiguration> babe_config_;
  std::shared_ptr<BabeConfigRepositoryMock> babe_config_repo_;
  std::shared_ptr<BlockValidatorMock> block_validator_;
  std::shared_ptr<EnvironmentMock> grandpa_environment_;
  std::shared_ptr<TransactionPoolMock> tx_pool_;
  std::shared_ptr<HasherMock> hasher_;
  std::shared_ptr<DigestTrackerMock> digest_tracker_;
  std::shared_ptr<BabeUtilMock> babe_util_;
  std::shared_ptr<OffchainWorkerApiMock> offchain_worker_api_;
  kagome::primitives::events::StorageSubscriptionEnginePtr storage_sub_engine_;
  kagome::primitives::events::ChainSubscriptionEnginePtr chain_sub_engine_;
  std::shared_ptr<ConsistencyKeeperMock> consistency_keeper_;

  std::shared_ptr<BlockExecutorImpl> block_executor_;
};

/**
 * For correct work of authority set changes, digests should be processed
 * after a justification is applied, if one is present.
 * Otherwise, a situation may occur where digests think that the current block
 * is not finalized and execute the wrong logic.
 */
TEST_F(BlockExecutorTest, JustificationFollowDigests) {
  AuthorityList authorities{Authority{{"auth0"_hash256}, 1},
                            Authority{{"auth1"_hash256}, 1}};
  kagome::primitives::BlockHeader header{
      .parent_hash = "parent_hash"_hash256,
      .number = 42,
      .digest = kagome::primitives::Digest{
          kagome::primitives::PreRuntime{{
              kagome::primitives::kBabeEngineId,
              Buffer{scale::encode(BabeBlockHeader{.authority_index = 1,
                                                   .slot_number = 1})
                         .value()},
          }},
          kagome::primitives::Consensus{
              kagome::primitives::ScheduledChange{authorities, 0}},
          kagome::primitives::Seal{{
              kagome::primitives::kBabeEngineId,
              Buffer{scale::encode(kagome::consensus::babe::Seal{}).value()},
          }}}};
  kagome::primitives::Justification justification{.data =
                                                      "justification_data"_buf};
  kagome::primitives::BlockData block_data{
      .hash = "some_block"_hash256,
      .header = header,
      .body = kagome::primitives::BlockBody{},
      .justification = justification};
  EXPECT_CALL(*block_tree_, getBlockBody("some_hash"_hash256))
      .WillOnce(
          testing::Return(kagome::blockchain::BlockTreeError::BODY_NOT_FOUND));
  EXPECT_CALL(*hasher_, blake2b_256(_))
      .WillOnce(testing::Return("some_hash"_hash256));

  babe_config_->leadership_rate.second = 42;
  EXPECT_CALL(*block_validator_,
              validateHeader(header,
                             1,
                             AuthorityId{"auth3"_hash256},
                             kagome::consensus::babe::calculateThreshold(
                                 babe_config_->leadership_rate, authorities, 0),
                             testing::Ref(*babe_config_)))
      .WillOnce(testing::Return(outcome::success()));
  EXPECT_CALL(*block_tree_, getBlockHeader("parent_hash"_hash256))
      .WillRepeatedly(testing::Return(kagome::primitives::BlockHeader{
          .parent_hash = "grandparent_hash"_hash256, .number = 40}));
  EXPECT_CALL(*block_tree_, getLastFinalized())
      .WillOnce(testing::Return(BlockInfo{40, "grandparent_hash"_hash256}))
      .WillOnce(testing::Return(BlockInfo{42, "some_hash"_hash256}));
  EXPECT_CALL(*block_tree_,
              getBestContaining("grandparent_hash"_hash256,
                                std::optional<BlockNumber>{}))
      .WillOnce(testing::Return(BlockInfo{41, "parent_hash"_hash256}));
  EXPECT_CALL(*core_, execute_block_ref(_, _))
      .WillOnce(testing::Return(outcome::success()));
  EXPECT_CALL(*block_tree_, addBlock(_))
      .WillOnce(testing::Return(outcome::success()));

  BlockInfo block_info{42, "some_hash"_hash256};

  {
    testing::InSequence s;

    EXPECT_CALL(
        *digest_tracker_,
        onDigest(BlockContext{.block_info = {42, "some_hash"_hash256}}, _))
        .WillOnce(testing::Return(outcome::success()));
  }
  EXPECT_CALL(
      *block_tree_,
      getBestContaining("some_hash"_hash256, std::optional<BlockNumber>{}))
      .WillOnce(testing::Return(BlockInfo{42, "some_hash"_hash256}));
  EXPECT_CALL(*offchain_worker_api_, offchain_worker(_, _))
      .WillOnce(testing::Return(outcome::success()));

  EXPECT_CALL(*consistency_keeper_, start(block_info))
      .WillOnce(testing::Invoke(
          [&] { return ConsistencyGuard(*consistency_keeper_, block_info); }));
  EXPECT_CALL(*consistency_keeper_, commit(block_info)).WillOnce(Return());
  EXPECT_CALL(*consistency_keeper_, rollback(block_info))
      .WillRepeatedly(Return());

block_executor_->applyBlock(
      Block{block_data.header.value(), block_data.body.value()}, justification, [](auto &&result) {
  EXPECT_OUTCOME_TRUE_1(result)      ;
      });
  
}
