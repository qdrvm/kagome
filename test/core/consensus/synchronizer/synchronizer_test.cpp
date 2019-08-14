/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/synchronizer/impl/synchronizer_impl.hpp"

#include <gtest/gtest.h>
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

using testing::_;

class SynchronizerTest : public testing::Test {
 public:
  void SetUp() override {
    block_.header.parent_hash.fill(2);

    EXPECT_CALL(*server_, onBlocksRequest(_));

    synchronizer_ =
        std::make_shared<SynchronizerImpl>(tree_, headers_, network_state_);
  }

  std::shared_ptr<BlockTreeMock> tree_ = std::make_shared<BlockTreeMock>();
  std::shared_ptr<HeaderRepositoryMock> headers_ =
      std::make_shared<HeaderRepositoryMock>();
  std::shared_ptr<PeerClientMock> peer1_ = std::make_shared<PeerClientMock>(),
                                  peer2_ = std::make_shared<PeerClientMock>();
  std::shared_ptr<PeerServerMock> server_ = std::make_shared<PeerServerMock>();
  std::shared_ptr<NetworkState> network_state_ = std::make_shared<NetworkState>(
      PeerClientsMap{{"peer1"_peerid, peer1_}, {"peer2"_peerid, peer2_}},
      server_);

  std::shared_ptr<Synchronizer> synchronizer_;

  Block block_{{{}, 2}, {{{0x11, 0x22}}, {{0x55, 0x66}}}};
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

TEST_F(SynchronizerTest, RequestWithoutHash) {}

TEST_F(SynchronizerTest, RequestWithHash) {}

TEST_F(SynchronizerTest, ProcessRequest) {}
