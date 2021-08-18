/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/sync_protocol_observer_impl.hpp"

#include <gtest/gtest.h>

#include <boost/optional.hpp>
#include <functional>

#include "application/app_configuration.hpp"
#include "mock/core/blockchain/block_header_repository_mock.hpp"
#include "mock/core/blockchain/block_tree_mock.hpp"
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
    sync_protocol_observer_ =
        std::make_shared<SyncProtocolObserverImpl>(tree_, headers_);
  }

  std::shared_ptr<HostMock> host_ = std::make_shared<HostMock>();
  PeerInfo peer_info_{"my_peer"_peerid, {}};

  std::shared_ptr<BlockTreeMock> tree_ = std::make_shared<BlockTreeMock>();
  std::shared_ptr<BlockHeaderRepositoryMock> headers_ =
      std::make_shared<BlockHeaderRepositoryMock>();

  std::shared_ptr<SyncProtocolObserver> sync_protocol_observer_;

  const Hash256 block2_hash_ = "2"_hash256;
  const Block block3_{{block2_hash_, 3}, {{{0x31, 0x32}}, {{0x33, 0x34}}}};
  const Hash256 block3_hash_ = "3"_hash256;
  const Block block4_{{block3_hash_, 4}, {{{0x41, 0x42}}, {{0x43, 0x44}}}};
  const Hash256 block4_hash_ = "4"_hash256;
};

/**
 * @given synchronizer
 * @when a request for blocks arrives
 * @then an expected response is formed @and sent
 */
TEST_F(SynchronizerTest, ProcessRequest) {
  // GIVEN
  BlocksRequest received_request{1,
                                 BlocksRequest::kBasicAttributes,
                                 block3_hash_,
                                 boost::none,
                                 Direction::ASCENDING,
                                 boost::none};

  EXPECT_CALL(
      *tree_,
      getChainByBlock(
          block3_hash_, true, AppConfiguration::kAbsolutMaxBlocksInResponse))
      .WillOnce(Return(std::vector<BlockHash>{block3_hash_, block4_hash_}));

  EXPECT_CALL(*headers_, getBlockHeader(BlockId{block3_hash_}))
      .WillOnce(Return(block3_.header));
  EXPECT_CALL(*headers_, getBlockHeader(BlockId{block4_hash_}))
      .WillOnce(Return(block4_.header));

  EXPECT_CALL(*tree_, getBlockBody(BlockId{block3_hash_}))
      .WillOnce(Return(block3_.body));
  EXPECT_CALL(*tree_, getBlockBody(BlockId{block4_hash_}))
      .WillOnce(Return(block4_.body));

  EXPECT_CALL(*tree_, getBlockJustification(BlockId{block3_hash_}))
      .WillOnce(Return(::outcome::failure(boost::system::error_code{})));
  EXPECT_CALL(*tree_, getBlockJustification(BlockId{block4_hash_}))
      .WillOnce(Return(::outcome::failure(boost::system::error_code{})));

  // WHEN
  EXPECT_OUTCOME_TRUE(
      response, sync_protocol_observer_->onBlocksRequest(received_request));

  // THEN
  ASSERT_EQ(response.id, received_request.id);

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
