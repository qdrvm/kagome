/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <chrono>

#include "consensus/babe/babe_util.hpp"
#include "consensus/babe/impl/babe_config_repository_impl.hpp"
#include "mock/core/application/app_state_manager_mock.hpp"
#include "mock/core/blockchain/block_header_repository_mock.hpp"
#include "mock/core/blockchain/block_tree_mock.hpp"
#include "mock/core/clock/clock_mock.hpp"
#include "mock/core/crypto/hasher_mock.hpp"
#include "mock/core/runtime/babe_api_mock.hpp"
#include "mock/core/storage/persistent_map_mock.hpp"
#include "mock/core/storage/spaced_storage_mock.hpp"
#include "primitives/babe_configuration.hpp"
#include "testutil/literals.hpp"
#include "testutil/prepare_loggers.hpp"

using namespace kagome;
using blockchain::BlockHeaderRepositoryMock;
using blockchain::BlockTreeMock;
using clock::SystemClockMock;
using common::Buffer;
using common::BufferView;
using consensus::babe::BabeConfigRepositoryImpl;
using consensus::babe::BabeUtil;
using crypto::HasherMock;
using primitives::BabeSlotNumber;
using primitives::BlockHeader;
using primitives::BlockId;
using primitives::BlockInfo;
using primitives::events::ChainSubscriptionEngine;
using runtime::BabeApiMock;
using storage::BufferStorageMock;
using storage::SpacedStorageMock;

using std::chrono_literals::operator""ms;

using testing::_;
using testing::Return;
using testing::ReturnRef;
using testing::ReturnRefOfCopy;

class BabeConfigRepositoryTest : public testing::Test {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {
    babe_config.slot_duration = 6000ms;
    babe_config.epoch_length = 2;

    app_state_manager = std::make_shared<application::AppStateManagerMock>();
    EXPECT_CALL(*app_state_manager, atPrepare(_)).WillOnce(Return());

    persistent_storage = std::make_shared<BufferStorageMock>();
    EXPECT_CALL(*persistent_storage, tryGetMock(_))
        .WillRepeatedly(Return(std::nullopt));

    spaced_storage = std::make_shared<SpacedStorageMock>();
    EXPECT_CALL(*spaced_storage, getSpace(_))
        .WillRepeatedly(Return(persistent_storage));

    block_tree = std::make_shared<BlockTreeMock>();
    EXPECT_CALL(*block_tree, getLastFinalized())
        .WillOnce(Return(BlockInfo{0, "genesis"_hash256}));
    EXPECT_CALL(*block_tree, getGenesisBlockHash())
        .WillOnce(testing::ReturnRefOfCopy("genesis"_hash256));

    header_repo = std::make_shared<BlockHeaderRepositoryMock>();

    babe_api = std::make_shared<BabeApiMock>();
    EXPECT_CALL(*babe_api, configuration(_))
        .WillRepeatedly(Return(babe_config));

    hasher = std::make_shared<HasherMock>();
    chain_events_engine = std::make_shared<ChainSubscriptionEngine>();
    clock = std::make_shared<SystemClockMock>();

    babe_config_repo_ =
        std::make_shared<BabeConfigRepositoryImpl>(*app_state_manager,
                                                   spaced_storage,
                                                   block_tree,
                                                   header_repo,
                                                   babe_api,
                                                   hasher,
                                                   chain_events_engine,
                                                   *clock);
  }

  primitives::BabeConfiguration babe_config;

  std::shared_ptr<application::AppStateManagerMock> app_state_manager;
  std::shared_ptr<SpacedStorageMock> spaced_storage;
  std::shared_ptr<BufferStorageMock> persistent_storage;
  std::shared_ptr<blockchain::BlockTreeMock> block_tree;
  std::shared_ptr<blockchain::BlockHeaderRepository> header_repo;
  std::shared_ptr<runtime::BabeApiMock> babe_api;
  std::shared_ptr<crypto::Hasher> hasher;
  primitives::events::ChainSubscriptionEnginePtr chain_events_engine;
  std::shared_ptr<SystemClockMock> clock;

  std::shared_ptr<BabeConfigRepositoryImpl> babe_config_repo_;
};

/**
 * @given current time
 * @when getCurrentSlot is called
 * @then compare slot estimations
 */
TEST_F(BabeConfigRepositoryTest, getCurrentSlot) {
  babe_config_repo_->prepare();
  auto time = std::chrono::system_clock::now();
  EXPECT_CALL(*clock, now()).Times(1).WillOnce(Return(time));
  EXPECT_EQ(static_cast<BabeSlotNumber>(time.time_since_epoch()
                                        / babe_config.slot_duration),
            babe_config_repo_->getCurrentSlot());
}
