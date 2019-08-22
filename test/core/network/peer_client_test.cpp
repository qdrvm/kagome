/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/peer_client_libp2p.hpp"

#include <memory>

#include <gtest/gtest.h>
#include <boost/optional.hpp>
#include "libp2p/multi/uvarint.hpp"
#include "libp2p/peer/peer_info.hpp"
#include "mock/libp2p/basic/message_read_writer_mock.hpp"
#include "mock/libp2p/connection/stream_mock.hpp"
#include "mock/libp2p/host/host_mock.hpp"
#include "network/impl/common.hpp"
#include "scale/scale.hpp"
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
  void SetUp() override {
    UVarint blocks_request_varint{encoded_blocks_request_.size()};
    encoded_blocks_request_.insert(encoded_blocks_request_.begin(),
                                   blocks_request_varint.toVector().begin(),
                                   blocks_request_varint.toVector().end());
  }

  HostMock host_;
  PeerInfo peer_info_{"my_peer"_peerid, {}};

  std::shared_ptr<PeerClient> peer_client_ =
      std::make_shared<PeerClientLibp2p<MessageReadWriterMock>>(host_,
                                                                peer_info_);

  std::shared_ptr<StreamMock> stream_ = std::make_shared<StreamMock>();
  std::shared_ptr<MessageReadWriterMock> read_writer_ =
      std::make_shared<MessageReadWriterMock>();

  BlocksRequest blocks_request_{
      42, 1, 2, boost::none, Direction::ASCENDING, 228};
  std::vector<uint8_t> encoded_blocks_request_ =
      scale::encode(blocks_request_).value();

  BlocksResponse blocks_response_{blocks_request_.id, {}};
  std::vector<uint8_t> encoded_blocks_response_ =
      scale::encode(blocks_response_).value();
  UVarint blocks_response_varint_{encoded_blocks_response_.size()};
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

  EXPECT_CALL(*read_writer_,
              write(encoded_blocks_request_, encoded_blocks_request_.size(), _))
      .WillOnce(InvokeArgument<2>(outcome::success()));
  EXPECT_CALL(*stream_, read(_, _, _))
      .WillOnce(InvokeArgument<2>(blocks_response_varint_.toBytes()))
      .WillOnce(InvokeArgument<2>(encoded_blocks_response_));

  // WHEN
  peer_client_->blocksRequest(blocks_request_, [this](auto &&response_res) {
    // THEN
    ASSERT_TRUE(response_res);
    ASSERT_EQ(response_res.value(), blocks_response_);
  });
}

/**
 * @given PeerClient on top of Libp2p
 * @when announcing a block
 * @then that announce is sent to the peer
 */
TEST_F(PeerClientTest, BlockAnnounce) {}
