/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/synchronizer/impl/synchronizer_impl.hpp"

#include <functional>

#include <gtest/gtest.h>
#include <boost/optional.hpp>
#include "mock/core/blockchain/block_tree_mock.hpp"
#include "mock/core/blockchain/header_backend_mock.hpp"
#include "mock/core/network/peer_client_mock.hpp"
#include "primitives/block.hpp"
#include "testutil/gmock_actions.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using namespace kagome;
using namespace blockchain;
using namespace network;
using namespace consensus;
using namespace primitives;
using namespace common;
using namespace libp2p::peer;

using testing::_;
using testing::Ref;
using testing::Return;
using testing::ReturnRef;

class SynchronizerTest : public testing::Test {
 public:
  void SetUp() override {
    block1_.header.parent_hash.fill(2);
    block1_hash_.fill(3);
    block2_.header.parent_hash = block1_hash_;
    block2_hash_.fill(4);

    synchronizer_ =
        std::make_shared<SynchronizerImpl>(peer_info_, tree_, headers_);
  }

  PeerInfo peer_info_{"my_peer"_peerid, {}};

  std::shared_ptr<BlockTreeMock> tree_ = std::make_shared<BlockTreeMock>();
  std::shared_ptr<HeaderRepositoryMock> headers_ =
      std::make_shared<HeaderRepositoryMock>();

  std::shared_ptr<Synchronizer> synchronizer_;

  Block block1_{{{}, 2}, {{{0x11, 0x22}}, {{0x55, 0x66}}}},
      block2_{{{}, 3}, {{{0x13, 0x23}}, {{0x35, 0x63}}}};
  Hash256 block1_hash_{}, block2_hash_{};
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
                                 block1_hash_,
                                 boost::none,
                                 Direction::DESCENDING,
                                 boost::none};

  EXPECT_CALL(*tree_, getChainByBlock(block1_hash_, false, 128))
      .WillOnce(Return(std::vector<BlockHash>{block1_hash_, block2_hash_}));

  EXPECT_CALL(*headers_, getBlockHeader(BlockId{block1_hash_}))
      .WillOnce(Return(block1_.header));
  EXPECT_CALL(*headers_, getBlockHeader(BlockId{block2_hash_}))
      .WillOnce(Return(block2_.header));

  EXPECT_CALL(*tree_, getBlockBody(BlockId{block1_hash_}))
      .WillOnce(Return(block1_.body));
  EXPECT_CALL(*tree_, getBlockBody(BlockId{block2_hash_}))
      .WillOnce(Return(block2_.body));

  EXPECT_CALL(*tree_, getBlockJustification(BlockId{block1_hash_}))
      .WillOnce(Return(outcome::failure(boost::system::error_code{})));
  EXPECT_CALL(*tree_, getBlockJustification(BlockId{block2_hash_}))
      .WillOnce(Return(outcome::failure(boost::system::error_code{})));

  // WHEN
  EXPECT_OUTCOME_TRUE(response,
                      synchronizer_->onBlocksRequest(received_request));

  // THEN
  ASSERT_EQ(response.id, 1);

  const auto &received_blocks = response.blocks;
  ASSERT_EQ(received_blocks.size(), 2);

  ASSERT_EQ(received_blocks[0].hash, block1_hash_);
  ASSERT_EQ(received_blocks[0].header, block1_.header);
  ASSERT_EQ(received_blocks[0].body, block1_.body);
  ASSERT_FALSE(received_blocks[0].justification);

  ASSERT_EQ(received_blocks[1].hash, block2_hash_);
  ASSERT_EQ(received_blocks[1].header, block2_.header);
  ASSERT_EQ(received_blocks[1].body, block2_.body);
  ASSERT_FALSE(received_blocks[1].justification);
}
