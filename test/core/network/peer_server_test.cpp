/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/peer_server_libp2p.hpp"

#include <memory>

#include <gtest/gtest.h>
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
using testing::NiceMock;

class PeerServerTest : public testing::Test {
 public:
  void SetUp() override {
    EXPECT_CALL(host_, setProtocolHandler(kSyncProtocol, _))
        .WillOnce(testing::SaveArg<1>(&sync_proto_handler_));
    EXPECT_CALL(host_, setProtocolHandler(kGossipProtocol, _))
        .WillOnce(testing::SaveArg<1>(&gossip_proto_handler_));

    peer_server_->start();
  }

  HostMock host_;
  PeerInfo peer_info_{"my_peer"_peerid, {}};

  std::shared_ptr<PeerServer> peer_server_ =
      std::make_shared<PeerServerLibp2p>(host_, peer_info_);

  std::shared_ptr<NiceMock<StreamMock>> stream_ =
      std::make_shared<NiceMock<StreamMock>>();

  std::function<connection::Stream::Handler> sync_proto_handler_;
  std::function<connection::Stream::Handler> gossip_proto_handler_;

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
 * @given PeerServer
 * @when subscribing to new BlocksRequests
 * @then subscriber receives a corresponding message, when it arrives, @and
 * PeerServer writes a response to it
 */
TEST_F(PeerServerTest, SyncProtoBlocksRequest) {
  setReadExpectations(stream_, encoded_blocks_request_);
  setWriteExpectations(stream_, encoded_blocks_response_);

  auto received = false;
  peer_server_->onBlocksRequest(
      [this, &received](const BlocksRequest &request) {
        received = true;
        return blocks_response_;
      });

  sync_proto_handler_(stream_);
  ASSERT_TRUE(received);
}

/**
 * @given PeerServer
 * @when subscribing to new BlocksRequests
 * @then subscriber receives nothing, when an unknown message arrives to the
 * server
 */
TEST_F(PeerServerTest, SyncProtoUnknownMessage) {
  setReadExpectations(stream_, std::vector<uint8_t>{0x11, 0x22, 0x33});

  auto received = false;
  peer_server_->onBlocksRequest([this, &received](const BlocksRequest &) {
    received = true;
    return blocks_response_;
  });

  sync_proto_handler_(stream_);
  ASSERT_FALSE(received);
}

/**
 * @given PeerServer
 * @when subscribing to new BlockAnnounces
 * @then subscriber receives a corresponding message, when it arrives
 */
TEST_F(PeerServerTest, GossipProtoBlockAnnounce) {
  setReadExpectations(stream_, encoded_announce);  // first call

  auto received = false;
  peer_server_->onBlockAnnounce([this, &received](const BlockAnnounce &) {
    EXPECT_CALL(*stream_, read(_, 1, _))  // second call
        .WillOnce(
            InvokeArgument<2>(outcome::failure(boost::system::error_code{})));
    received = true;
  });

  gossip_proto_handler_(stream_);
  ASSERT_TRUE(received);
}

/**
 * @given PeerServer
 * @when subscribing to new BlockAnnounces
 * @then subscriber receives nothing, when an unknown message arrives to the
 * server
 */
TEST_F(PeerServerTest, GossipProtoUknownMessage) {
  setReadExpectations(stream_, std::vector<uint8_t>{0x11, 0x22, 0x33});

  auto received = false;
  peer_server_->onBlockAnnounce(
      [&received](const BlockAnnounce &) { received = true; });

  gossip_proto_handler_(stream_);
  ASSERT_FALSE(received);
}
