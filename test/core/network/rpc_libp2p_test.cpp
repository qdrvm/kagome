/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/rpc_libp2p.hpp"

#include <gtest/gtest.h>
#include "libp2p/basic/scale_message_read_writer.hpp"
#include "mock/libp2p/basic/read_writer_mock.hpp"
#include "mock/libp2p/connection/stream_mock.hpp"
#include "mock/libp2p/host/host_mock.hpp"
#include "network/types/block_response.hpp"
#include "scale/scale.hpp"
#include "testutil/libp2p/message_read_writer_helper.hpp"
#include "testutil/literals.hpp"

using namespace kagome;
using namespace network;

using namespace libp2p;
using namespace basic;
using namespace peer;
using namespace connection;

using testing::_;

using ScaleRPC = RPCLibp2p<ScaleMessageReadWriter>;

class RpcLibp2pTest : public testing::Test {
 public:
  std::shared_ptr<ReadWriterMock> read_writer_ =
      std::make_shared<ReadWriterMock>();
  std::shared_ptr<StreamMock> stream_ = std::make_shared<StreamMock>();

  HostMock host_;
  PeerInfo peer_info_{"my_peer"_peerid, {}};
  Protocol protocol_{"/test/2.2.8"};

  // we are not interested in the exact semantics, so let BlockResponse be both
  // request and response
  BlockResponse request_{1}, response_{2};
  common::Buffer encoded_request_{scale::encode(request_).value()},
      encoded_response_{scale::encode(response_).value()};
};

/**
 * @given RPCLibp2p
 * @when reading a message @and answering with a response
 * @then operation completes successfully
 */
TEST_F(RpcLibp2pTest, ReadWithResponse) {
  setReadExpectations(read_writer_, encoded_request_.toVector());
  setWriteExpectations(read_writer_, encoded_response_.toVector());

  auto finished = false;
  ScaleRPC::read<BlockResponse, BlockResponse>(
      read_writer_,
      [this, &finished](auto &&received_request) mutable {
        EXPECT_EQ(received_request.id, request_.id);
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
  setReadExpectations(read_writer_, encoded_request_.toVector());

  auto finished = false;
  ScaleRPC::read<BlockResponse, BlockResponse>(
      read_writer_,
      [this](auto &&received_request) {
        EXPECT_EQ(received_request.id, request_.id);
        return outcome::failure(boost::system::error_code{});
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
  setReadExpectations(read_writer_, encoded_request_.toVector());

  auto finished = false;
  ScaleRPC::read<BlockResponse>(
      read_writer_,
      [this, &finished](auto &&received_request) mutable {
        EXPECT_EQ(received_request.id, request_.id);
        finished = true;
      },
      [](auto &&err) { FAIL() << err.error().message(); });

  ASSERT_TRUE(finished);
}

/**
 * @given RPCLibp2p
 * @when writing a message @and waiting for a response
 * @then response is received
 */
TEST_F(RpcLibp2pTest, WriteWithResponse) {
  EXPECT_CALL(host_, newStream(peer_info_, protocol_, _))
      .WillOnce(testing::InvokeArgument<2>(stream_));

  setWriteExpectations(stream_, encoded_request_.toVector());
  setReadExpectations(stream_, encoded_response_.toVector());

  auto finished = false;
  ScaleRPC::write<BlockResponse, BlockResponse>(
      host_,
      peer_info_,
      protocol_,
      request_,
      [this, &finished](auto &&response_res) mutable {
        ASSERT_TRUE(response_res);
        ASSERT_EQ(response_res.value().id, response_.id);
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
  EXPECT_CALL(host_, newStream(peer_info_, protocol_, _))
      .WillOnce(testing::InvokeArgument<2>(stream_));

  setWriteExpectations(stream_, encoded_request_.toVector());
  EXPECT_CALL(*stream_, read(_, _, _))
      .WillOnce(testing::InvokeArgument<2>(
          outcome::failure(boost::system::error_code{})));

  auto finished = false;
  ScaleRPC::write<BlockResponse, BlockResponse>(
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
  EXPECT_CALL(host_, newStream(peer_info_, protocol_, _))
      .WillOnce(testing::InvokeArgument<2>(stream_));

  setWriteExpectations(stream_, encoded_request_.toVector());

  auto finished = false;
  ScaleRPC::write<BlockResponse>(host_,
                                 peer_info_,
                                 protocol_,
                                 request_,
                                 [&finished](auto &&write_res) mutable {
                                   ASSERT_TRUE(write_res);
                                   finished = true;
                                 });

  ASSERT_TRUE(finished);
}
