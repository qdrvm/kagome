/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/synchronizer/impl/synchronizer_impl.hpp"

#include <gtest/gtest.h>
#include <boost/optional.hpp>
#include "mock/core/blockchain/block_tree_mock.hpp"
#include "mock/core/blockchain/header_backend_mock.hpp"
#include "mock/core/network/peer_client_mock.hpp"
#include "mock/core/network/peer_server_mock.hpp"
#include "network/network_state.hpp"
#include "primitives/block.hpp"
#include "testutil/gmock_actions.hpp"
#include "testutil/literals.hpp"

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

class SynchronizerTest : public testing::Test {
 public:
  void SetUp() override {
    block_.header.parent_hash.fill(2);
    block_hash_.fill(3);

    EXPECT_CALL(*server_, onBlocksRequest(_));

    synchronizer_ =
        std::make_shared<SynchronizerImpl>(tree_, headers_, network_state_);
  }

  PeerInfo peer1_info_{"peer1"_peerid, {}}, peer2_info_{"peer2"_peerid, {}};

  std::shared_ptr<BlockTreeMock> tree_ = std::make_shared<BlockTreeMock>();
  std::shared_ptr<HeaderRepositoryMock> headers_ =
      std::make_shared<HeaderRepositoryMock>();
  std::shared_ptr<PeerClientMock> peer1_ = std::make_shared<PeerClientMock>(),
                                  peer2_ = std::make_shared<PeerClientMock>();
  std::shared_ptr<PeerServerMock> server_ = std::make_shared<PeerServerMock>();
  std::shared_ptr<NetworkState> network_state_ = std::make_shared<NetworkState>(
      PeerClientsMap{{peer1_info_.id, peer1_}, {peer2_info_.id, peer2_}},
      server_);

  std::shared_ptr<Synchronizer> synchronizer_;

  Block block_{{{}, 2}, {{{0x11, 0x22}}, {{0x55, 0x66}}}};
  Hash256 block_hash_{};
};

/**
 * @given synchronizer
 * @when announcing a block header
 * @then all peers receive a corresponding message
 */
TEST_F(SynchronizerTest, Announce) {
  // GIVEN
  EXPECT_CALL(*peer1_, blockAnnounce(BlockAnnounce{block_.header}, _))
      .WillOnce(Arg1CallbackWithArg(outcome::success()));
  EXPECT_CALL(*peer2_, blockAnnounce(BlockAnnounce{block_.header}, _))
      .WillOnce(Arg1CallbackWithArg(outcome::success()));

  // WHEN
  synchronizer_->announce(block_.header);

  // THEN
}

TEST_F(SynchronizerTest, RequestWithoutHash) {
  // GIVEN
  EXPECT_CALL(*tree_, deepestLeaf()).WillOnce(Return(Ref(block_hash_)));
  BlockRequest expected_request{0,
                                BlockRequest::kBasicAttributes,
                                block_hash_,
                                boost::none,
                                Direction::DESCENDING,
                                boost::none};

  EXPECT_CALL(*peer1_, blocksRequest(expected_request, _))
      .WillOnce(Arg1CallbackWithArg(outcome::success()));

  // WHEN
  auto finished = false;
  synchronizer_->requestBlocks(peer1_info_, [&finished](auto &&res) mutable {
    ASSERT_TRUE(res);
    finished = true;
  });

  // THEN
  ASSERT_TRUE(finished);
}

TEST_F(SynchronizerTest, RequestWithHash) {}

TEST_F(SynchronizerTest, ProcessRequest) {}
