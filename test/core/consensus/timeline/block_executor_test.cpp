/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/timeline/impl/block_executor_impl.hpp"

#include <gtest/gtest.h>
#include <iostream>

#include "blockchain/block_tree_error.hpp"
#include "common/worker_thread_pool.hpp"
#include "consensus/babe/impl/threshold_util.hpp"
#include "consensus/babe/types/seal.hpp"
#include "consensus/timeline/impl/block_appender_base.hpp"
#include "mock/core/blockchain/block_tree_mock.hpp"
#include "mock/core/consensus/babe/babe_config_repository_mock.hpp"
#include "mock/core/consensus/grandpa/environment_mock.hpp"
#include "mock/core/consensus/production_consensus_mock.hpp"
#include "mock/core/consensus/timeline/consensus_selector_mock.hpp"
#include "mock/core/consensus/timeline/slots_util_mock.hpp"
#include "mock/core/crypto/hasher_mock.hpp"
#include "mock/core/runtime/core_mock.hpp"
#include "mock/core/runtime/offchain_worker_api_mock.hpp"
#include "mock/core/transaction_pool/transaction_pool_mock.hpp"
#include "testutil/lazy.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/prepare_loggers.hpp"
#include "utils/watchdog.hpp"

using kagome::Watchdog;
using kagome::TestThreadPool;
using kagome::blockchain::BlockTree;
using kagome::blockchain::BlockTreeError;
using kagome::blockchain::BlockTreeMock;
using kagome::common::Buffer;
using kagome::common::WorkerThreadPool;
using kagome::consensus::BlockAppenderBase;
using kagome::consensus::BlockExecutorImpl;
using kagome::consensus::ConsensusSelector;
using kagome::consensus::ConsensusSelectorMock;
using kagome::consensus::EpochNumber;
using kagome::consensus::EpochTimings;
using kagome::consensus::ProductionConsensusMock;
using kagome::consensus::SlotsUtil;
using kagome::consensus::SlotsUtilMock;
using BabeAuthority = kagome::consensus::babe::Authority;
using BabeAuthorityId = kagome::consensus::babe::AuthorityId;
using BabeAuthorities = kagome::consensus::babe::Authorities;
using kagome::consensus::babe::BabeBlockHeader;
using kagome::consensus::babe::BabeConfigRepositoryMock;
using kagome::consensus::babe::BabeConfiguration;
using GrandpaAuthority = kagome::consensus::grandpa::Authority;
using GrandpaAuthorityId = kagome::consensus::grandpa::AuthorityId;
using GrandpaAuthorities = kagome::consensus::grandpa::Authorities;
using kagome::consensus::grandpa::Environment;
using kagome::consensus::grandpa::EnvironmentMock;
using kagome::consensus::grandpa::ForcedChange;
using kagome::consensus::grandpa::OnDisabled;
using kagome::consensus::grandpa::Pause;
using kagome::consensus::grandpa::Resume;
using kagome::consensus::grandpa::ScheduledChange;
using kagome::crypto::Hasher;
using kagome::crypto::HasherMock;
using kagome::crypto::VRFThreshold;
using kagome::primitives::Block;
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
  std::ostream &operator<<(std::ostream &s, const detail::DigestItemCommon &) {
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

inline BabeAuthorityId operator"" _babe_auth(const char *c, size_t s) {
  BabeAuthorityId res;
  std::copy_n(c, std::min(s, res.size()), res.begin());
  return res;
}

inline GrandpaAuthorityId operator"" _gran_auth(const char *c, size_t s) {
  GrandpaAuthorityId res;
  std::copy_n(c, std::min(s, res.size()), res.begin());
  return res;
}

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
    babe_config_->authorities = {BabeAuthority{"auth2"_babe_auth, 1},
                                 BabeAuthority{"auth3"_babe_auth, 1}};
    babe_config_->randomness = "randomness"_hash256;

    babe_config_repo_ = std::make_shared<BabeConfigRepositoryMock>();
    ON_CALL(*babe_config_repo_, config(_, _))
        .WillByDefault(Return(babe_config_));

    grandpa_environment_ = std::make_shared<EnvironmentMock>();
    tx_pool_ = std::make_shared<TransactionPoolMock>();
    hasher_ = std::make_shared<HasherMock>();

    slots_util_ = std::make_shared<SlotsUtilMock>();
    ON_CALL(*slots_util_, slotToEpoch(_, _)).WillByDefault(Return(1));

    production_consensus_ = std::make_shared<ProductionConsensusMock>();

    consensus_selector_ = std::make_shared<ConsensusSelectorMock>();
    ON_CALL(*consensus_selector_, getProductionConsensusByInfo(_))
        .WillByDefault(Return(production_consensus_));
    ON_CALL(*consensus_selector_, getProductionConsensusByHeader(_))
        .WillByDefault(Return(production_consensus_));

    offchain_worker_api_ = std::make_shared<OffchainWorkerApiMock>();
    storage_sub_engine_ = std::make_shared<
        kagome::primitives::events::StorageSubscriptionEngine>();
    chain_sub_engine_ =
        std::make_shared<kagome::primitives::events::ChainSubscriptionEngine>();

    auto appender = std::make_unique<BlockAppenderBase>(
        block_tree_,
        babe_config_repo_,
        timings_,
        grandpa_environment_,
        testutil::sptr_to_lazy<SlotsUtil>(slots_util_),
        hasher_,
        testutil::sptr_to_lazy<ConsensusSelector>(consensus_selector_));

    block_executor_ =
        std::make_shared<BlockExecutorImpl>(block_tree_,
                                            worker_thread_pool_,
                                            worker_thread_pool_->io_context(),
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
  EpochTimings timings_{
      .slot_duration = 60ms,
      .epoch_length = 2,
  };
  std::shared_ptr<EnvironmentMock> grandpa_environment_;
  std::shared_ptr<TransactionPoolMock> tx_pool_;
  std::shared_ptr<HasherMock> hasher_;
  std::shared_ptr<SlotsUtilMock> slots_util_;
  std::shared_ptr<ConsensusSelectorMock> consensus_selector_;
  std::shared_ptr<ProductionConsensusMock> production_consensus_;
  std::shared_ptr<OffchainWorkerApiMock> offchain_worker_api_;
  kagome::primitives::events::StorageSubscriptionEnginePtr storage_sub_engine_;
  kagome::primitives::events::ChainSubscriptionEnginePtr chain_sub_engine_;
  std::shared_ptr<boost::asio::io_context> io_ =
      std::make_shared<boost::asio::io_context>();
  std::shared_ptr<Watchdog> watchdog_ = std::make_shared<Watchdog>();
  std::shared_ptr<WorkerThreadPool> worker_thread_pool_ =
      std::make_shared<WorkerThreadPool>(watchdog_);

  std::shared_ptr<BlockExecutorImpl> block_executor_;
};

/**
 * For correct work of authority set changes, digests should be processed
 * after a justification is applied, if one is present.
 * Otherwise, a situation may occur where digests think that the current block
 * is not finalized and execute the wrong logic.
 */
TEST_F(BlockExecutorTest, JustificationFollowDigests) {
  BabeAuthorities babe_authorities{BabeAuthority{"auth0"_babe_auth, 1},
                                   BabeAuthority{"auth1"_babe_auth, 1}};
  GrandpaAuthorities authorities{GrandpaAuthority{"auth0"_gran_auth, 1},
                                 GrandpaAuthority{"auth1"_gran_auth, 1}};
  kagome::primitives::BlockHash parent_hash = "parent_hash"_hash256;
  kagome::primitives::BlockHash some_hash = "some_hash"_hash256;

  kagome::primitives::BlockHeader header{
      42,           // number
      parent_hash,  // parent
      {},           // state root
      {},           // extrinsics root
      kagome::primitives::Digest{
          kagome::primitives::PreRuntime{{
              kagome::primitives::kBabeEngineId,
              Buffer{scale::encode(BabeBlockHeader{.authority_index = 1,
                                                   .slot_number = 1})
                         .value()},
          }},
          kagome::primitives::Consensus{ScheduledChange{authorities, 0}},
          kagome::primitives::Seal{{
              kagome::primitives::kBabeEngineId,
              Buffer{scale::encode(kagome::consensus::babe::Seal{}).value()},
          }}},
      some_hash  // hash
  };

  kagome::primitives::Justification justification{.data =
                                                      "justification_data"_buf};
  kagome::primitives::BlockData block_data{
      .hash = header.hash(),
      .header = header,
      .body = kagome::primitives::BlockBody{},
      .justification = justification};
  EXPECT_CALL(*block_tree_, getBlockBody(some_hash))
      .WillOnce(
          testing::Return(kagome::blockchain::BlockTreeError::BODY_NOT_FOUND));

  babe_config_->leadership_rate.second = 42;
  EXPECT_CALL(*production_consensus_, validateHeader(header))
      .WillOnce(testing::Return(outcome::success()));
  EXPECT_CALL(*block_tree_, getBlockHeader(parent_hash))
      .WillRepeatedly(testing::Return(kagome::primitives::BlockHeader{
          40, "grandparent_hash"_hash256, {}, {}, {}}));
  EXPECT_CALL(*block_tree_, has(parent_hash)).WillRepeatedly(Return(true));
  EXPECT_CALL(*block_tree_, bestBlock())
      // previous best
      .WillOnce(testing::Return(BlockInfo{41, parent_hash}))
      // current best
      .WillOnce(testing::Return(BlockInfo{42, some_hash}));
  EXPECT_CALL(*core_, execute_block_ref(_, _))
      .WillOnce(testing::Return(outcome::success()));
  EXPECT_CALL(*block_tree_, addBlock(_))
      .WillOnce(testing::Return(outcome::success()));

  EXPECT_CALL(*offchain_worker_api_, offchain_worker(_, _))
      .WillOnce(testing::Return(outcome::success()));

  block_executor_->applyBlock(
      Block{block_data.header.value(), block_data.body.value()},
      justification,
      [&](auto &&result) { ASSERT_OUTCOME_SUCCESS_TRY(result); });

  io_->run();
}
