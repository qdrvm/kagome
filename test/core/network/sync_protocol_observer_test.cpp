/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/sync_protocol_observer_impl.hpp"

#include <gtest/gtest.h>

#include <functional>
#include <optional>

#include "application/app_configuration.hpp"
#include "mock/core/blockchain/block_header_repository_mock.hpp"
#include "mock/core/blockchain/block_tree_mock.hpp"
#include "mock/core/network/beefy.hpp"
#include "mock/core/network/peer_manager_mock.hpp"
#include "mock/libp2p/host/host_mock.hpp"
#include "primitives/block.hpp"
#include "testutil/gmock_actions.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/prepare_loggers.hpp"

using namespace kagome;
using namespace blockchain;
using namespace network;
using namespace primitives;
using namespace common;
using application::AppConfiguration;

using namespace libp2p;
using namespace peer;

using testing::_;
using testing::Ref;
using testing::Return;
using testing::ReturnRef;

class SynchronizerTest : public testing::Test {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {
    peer_manager_mock_ = std::make_shared<PeerManagerMock>();
    sync_protocol_observer_ = std::make_shared<SyncProtocolObserverImpl>(
        tree_, headers_, beefy_, peer_manager_mock_);
  }

  std::shared_ptr<HostMock> host_ = std::make_shared<HostMock>();
  PeerInfo peer_info_{"my_peer"_peerid, {}};

  std::shared_ptr<BlockTreeMock> tree_ = std::make_shared<BlockTreeMock>();
  std::shared_ptr<BlockHeaderRepositoryMock> headers_ =
      std::make_shared<BlockHeaderRepositoryMock>();

  std::shared_ptr<SyncProtocolObserver> sync_protocol_observer_;
  std::shared_ptr<PeerManagerMock> peer_manager_mock_;
  std::shared_ptr<BeefyMock> beefy_ = std::make_shared<BeefyMock>();

  const Hash256 block2_hash_ = "2"_hash256;
  const Block block3_{{3, block2_hash_, {}, {}, {}},
                      {{{0x31, 0x32}}, {{0x33, 0x34}}}};
  const Hash256 block3_hash_ = "3"_hash256;
  const Block block4_{{4, block3_hash_, {}, {}, {}},
                      {{{0x41, 0x42}}, {{0x43, 0x44}}}};
  const Hash256 block4_hash_ = "4"_hash256;
};

/**
 * @given synchronizer
 * @when a request for blocks arrives
 * @then an expected response is formed @and sent
 */
TEST_F(SynchronizerTest, ProcessRequest) {
  // GIVEN
  BlocksRequest received_request{BlocksRequest::kBasicAttributes,
                                 block3_hash_,
                                 Direction::ASCENDING,
                                 std::nullopt};

  EXPECT_CALL(*tree_,
              getBestChainFromBlock(
                  block3_hash_, AppConfiguration::kAbsolutMaxBlocksInResponse))
      .WillOnce(Return(std::vector<BlockHash>{block3_hash_, block4_hash_}));

  EXPECT_CALL(*headers_, getBlockHeader(block3_hash_))
      .WillOnce(Return(block3_.header));
  EXPECT_CALL(*headers_, getBlockHeader(block4_hash_))
      .WillOnce(Return(block4_.header));

  EXPECT_CALL(*tree_, getBlockBody(block3_hash_))
      .WillOnce(Return(block3_.body));
  EXPECT_CALL(*tree_, getBlockBody(block4_hash_))
      .WillOnce(Return(block4_.body));

  EXPECT_CALL(*tree_, getBlockJustification(block3_hash_))
      .WillOnce(Return(::outcome::failure(boost::system::error_code{})));
  EXPECT_CALL(*tree_, getBlockJustification(block4_hash_))
      .WillOnce(Return(::outcome::failure(boost::system::error_code{})));
  EXPECT_CALL(*peer_manager_mock_, reserveStatusStreams(peer_info_.id));

  EXPECT_CALL(*beefy_, getJustification(_)).WillRepeatedly([] {
    return ::outcome::success(std::nullopt);
  });

  // WHEN
  EXPECT_OUTCOME_TRUE(response,
                      sync_protocol_observer_->onBlocksRequest(received_request,
                                                               peer_info_.id));

  // THEN
  const auto &received_blocks = response.blocks;
  ASSERT_EQ(received_blocks.size(), 2);

  ASSERT_EQ(received_blocks[0].hash, block3_hash_);
  ASSERT_EQ(received_blocks[0].header, block3_.header);
  ASSERT_EQ(received_blocks[0].body, block3_.body);
  ASSERT_FALSE(received_blocks[0].justification);

  ASSERT_EQ(received_blocks[1].hash, block4_hash_);
  ASSERT_EQ(received_blocks[1].header, block4_.header);
  ASSERT_EQ(received_blocks[1].body, block4_.body);
  ASSERT_FALSE(received_blocks[1].justification);
}
