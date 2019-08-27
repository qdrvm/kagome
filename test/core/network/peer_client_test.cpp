/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/peer_client_libp2p.hpp"

#include <memory>

#include <gtest/gtest.h>
#include <boost/optional.hpp>
#include "libp2p/peer/peer_info.hpp"
#include "mock/libp2p/connection/stream_mock.hpp"
#include "mock/libp2p/host/host_mock.hpp"
#include "network/impl/common.hpp"
#include "scale/scale.hpp"
#include "testutil/libp2p/message_read_writer_helper.hpp"
#include "testutil/literals.hpp"

using namespace kagome;
using namespace network;

using namespace libp2p;
using namespace peer;
using namespace connection;
using namespace multi;
using namespace basic;

using testing::_;
using testing::InvokeArgument;

class PeerClientTest : public testing::Test {
 public:
  HostMock host_;
  PeerInfo peer_info_{"my_peer"_peerid, {}};

  std::shared_ptr<PeerClient> peer_client_ =
      std::make_shared<PeerClientLibp2p>(host_, peer_info_);

  std::shared_ptr<StreamMock> stream_ = std::make_shared<StreamMock>();

  BlocksRequest blocks_request_{
      42, {1}, 2, boost::none, Direction::ASCENDING, 228};
  std::vector<uint8_t> encoded_blocks_request_ =
      scale::encode(blocks_request_).value();

  BlocksResponse blocks_response_{blocks_request_.id, {}};
  std::vector<uint8_t> encoded_blocks_response_ =
      scale::encode(blocks_response_).value();

  BlockAnnounce announce{{{}, 42}};
  std::vector<uint8_t> encoded_announce = scale::encode(announce).value();
};

/**
 * @given PeerClient on top of Libp2pq
 * @when requesting a block
 * @then that request is sent to the peer @and response is being waited
 */
TEST_F(PeerClientTest, BlocksRequest) {
  // GIVEN
  EXPECT_CALL(host_, newStream(peer_info_, kSyncProtocol, _))
      .WillOnce(InvokeArgument<2>(stream_));

  setWriteExpectations(stream_, encoded_blocks_request_);
  setReadExpectations(stream_, encoded_blocks_response_);

  // WHEN
  auto finished = false;
  peer_client_->blocksRequest(
      blocks_request_, [this, &finished](auto &&response_res) mutable {
        // THEN
        ASSERT_TRUE(response_res);
        ASSERT_EQ(response_res.value(), blocks_response_);
        finished = true;
      });
  ASSERT_TRUE(finished);
}

/**
 * @given PeerClient on top of Libp2p
 * @when announcing a block
 * @then that announce is sent to the peer
 */
TEST_F(PeerClientTest, BlockAnnounce) {
  // GIVEN
  EXPECT_CALL(host_, newStream(peer_info_, kGossipProtocol, _))
      .WillOnce(InvokeArgument<2>(stream_));

  setWriteExpectations(stream_, encoded_announce);

  // WHEN
  auto finished = false;
  peer_client_->blockAnnounce(announce,
                              [&finished](auto &&response_res) mutable {
                                ASSERT_TRUE(response_res);
                                finished = true;
                              });
  ASSERT_TRUE(finished);
}
