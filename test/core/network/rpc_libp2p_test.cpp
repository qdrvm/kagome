/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/rpc.hpp"

#include <gtest/gtest.h>

#include "mock/libp2p/basic/read_writer_mock.hpp"
#include "mock/libp2p/connection/stream_mock.hpp"
#include "mock/libp2p/host/host_mock.hpp"
#include "network/helpers/scale_message_read_writer.hpp"
#include "network/types/blocks_response.hpp"
#include "scale/scale.hpp"
#include "testutil/libp2p/message_read_writer_helper.hpp"
#include "testutil/literals.hpp"
#include "testutil/prepare_loggers.hpp"

using namespace kagome;
using namespace network;

using namespace libp2p;
using namespace basic;
using namespace peer;
using namespace connection;

using testing::_;

using ScaleRPC = RPC<ScaleMessageReadWriter>;

namespace kagome::network {
  /// outputs object of type BlockResponse to stream
  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const BlocksResponse &v) {
    return s << v.blocks;
  }

  /// decodes object of type BlockResponse from stream
  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, BlocksResponse &v) {
    return s >> v.blocks;
  }
}  // namespace kagome::network

class RpcLibp2pTest : public testing::Test {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  std::shared_ptr<ReadWriterMock> read_writer_ =
      std::make_shared<ReadWriterMock>();
  std::shared_ptr<StreamMock> stream_ = std::make_shared<StreamMock>();

  HostMock host_;
  PeerInfo peer_info_{"my_peer"_peerid, {}};
  Protocol protocol_{"/test/2.2.8"};

  // we are not interested in the exact semantics, so let BlocksResponse be both
  // request and response
  BlocksResponse request_{.blocks = {primitives::BlockData{}}};
  BlocksResponse response_{.blocks = {primitives::BlockData{}}};
  kagome::common::Buffer encoded_request_{scale::encode(request_).value()};
  kagome::common::Buffer encoded_response_{scale::encode(response_).value()};
};

/**
 * @given RPCLibp2p
 * @when reading a message @and answering with a response
 * @then operation completes successfully
 */
TEST_F(RpcLibp2pTest, ReadWithResponse) {
  setReadExpectations(read_writer_, encoded_request_.asVector());
  setWriteExpectations(read_writer_, encoded_response_.asVector());

  auto finished = false;
  ScaleRPC::read<BlocksResponse, BlocksResponse>(
      read_writer_,
      [this, &finished](auto &&received_request) mutable {
        finished = true;
        return response_;
      },
      [](auto &&err) { FAIL() << err.error().message(); });

  ASSERT_TRUE(finished);
}

/**
 * @given RPCLibp2p
 * @when reading a message @and answering with an error
 * @then that error is properly handled
 */
TEST_F(RpcLibp2pTest, ReadWithResponseErroredResponse) {
  setReadExpectations(read_writer_, encoded_request_.asVector());

  auto finished = false;
  ScaleRPC::read<BlocksResponse, BlocksResponse>(
      read_writer_,
      [](auto &&received_request) {
        return ::outcome::failure(boost::system::error_code{});
      },
      [&finished](auto &&err) mutable { finished = true; });

  ASSERT_TRUE(finished);
}

/**
 * @given RPCLibp2p
 * @when reading a message without waiting for a response
 * @then operation completes successfully
 */
TEST_F(RpcLibp2pTest, ReadWithoutResponse) {
  setReadExpectations(read_writer_, encoded_request_.asVector());

  auto finished = false;
  ScaleRPC::read<BlocksResponse>(read_writer_,
                                 [&finished](auto &&received_request) mutable {
                                   EXPECT_TRUE(received_request);
                                   finished = true;
                                 });

  ASSERT_TRUE(finished);
}

/**
 * @given RPCLibp2p
 * @when writing a message @and waiting for a response
 * @then response is received
 */
TEST_F(RpcLibp2pTest, WriteWithResponse) {
  EXPECT_CALL(host_,
              newStream(peer_info_.id, libp2p::StreamProtocols{protocol_}, _))
      .WillOnce(
          testing::InvokeArgument<2>(StreamAndProtocol{stream_, protocol_}));

  setWriteExpectations(stream_, encoded_request_.asVector());
  setReadExpectations(stream_, encoded_response_.asVector());

  auto finished = false;
  ScaleRPC::write<BlocksResponse, BlocksResponse>(
      host_,
      peer_info_,
      protocol_,
      request_,
      [&finished](auto &&response_res) mutable {
        ASSERT_TRUE(response_res);
        finished = true;
      });

  ASSERT_TRUE(finished);
}

/**
 * @given RPCLibp2p
 * @when writing a message @and waiting for a response @and the error arrives
 * @then the error is properly handled
 */
TEST_F(RpcLibp2pTest, WriteWithResponseErroredResponse) {
  EXPECT_CALL(host_,
              newStream(peer_info_.id, libp2p::StreamProtocols{protocol_}, _))
      .WillOnce(
          testing::InvokeArgument<2>(StreamAndProtocol{stream_, protocol_}));

  setWriteExpectations(stream_, encoded_request_.asVector());
  EXPECT_CALL(*stream_, read(_, _, _))
      .WillOnce(testing::InvokeArgument<2>(
          ::outcome::failure(boost::system::error_code{})));

  auto finished = false;
  ScaleRPC::write<BlocksResponse, BlocksResponse>(
      host_,
      peer_info_,
      protocol_,
      request_,
      [&finished](auto &&response_res) mutable {
        ASSERT_FALSE(response_res);
        finished = true;
      });

  ASSERT_TRUE(finished);
}

/**
 * @given RPCLibp2p
 * @when writing a message without waiting for a response
 * @then operation completes successfully
 */
TEST_F(RpcLibp2pTest, WriteWithoutResponse) {
  EXPECT_CALL(host_,
              newStream(peer_info_.id, libp2p::StreamProtocols{protocol_}, _))
      .WillOnce(
          testing::InvokeArgument<2>(StreamAndProtocol{stream_, protocol_}));

  setWriteExpectations(stream_, encoded_request_.asVector());

  auto finished = false;
  ScaleRPC::write<BlocksResponse>(host_,
                                  peer_info_,
                                  protocol_,
                                  request_,
                                  [&finished](auto &&write_res) mutable {
                                    ASSERT_TRUE(write_res);
                                    finished = true;
                                  });

  ASSERT_TRUE(finished);
}
