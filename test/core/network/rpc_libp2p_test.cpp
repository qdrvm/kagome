/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/rpc_libp2p.hpp"

#include <gtest/gtest.h>
#include "libp2p/basic/scale_message_read_writer.hpp"
#include "mock/libp2p/basic/read_writer_mock.hpp"
#include "network/types/block_response.hpp"
#include "scale/scale.hpp"

using namespace kagome;
using namespace network;

using namespace libp2p;
using namespace basic;

using ScaleRPC = RPCLibp2p<ScaleMessageReadWriter>;

class RpcLibp2pTest : public testing::Test {
 public:
  std::shared_ptr<ReadWriterMock> read_writer_ =
      std::make_shared<ReadWriterMock>();

  // we are not interested in the exact semantics, so let BlockResponse be both
  // request and response
  BlockResponse request{1}, response{2};
  common::Buffer encoded_request{scale::encode(request).value()},
      encoded_response{scale::encode(response).value()};
};

/**
 * @given RPCLibp2p
 * @when reading a message @and answering with a response
 * @then operation completes successfully
 */
TEST_F(RpcLibp2pTest, ReadWithResponse) {
  ScaleRPC::read<BlockResponse, BlockResponse>(
      read_writer_,
      [this](auto &&received_request) {
        EXPECT_EQ(received_request.id, request.id);
        return response;
      },
      [](auto &&err) { FAIL() << err.error().message(); });
}

/**
 * @given RPCLibp2p
 * @when reading a message @and answering with an error
 * @then that error is properly handled
 */
TEST_F(RpcLibp2pTest, ReadWithResponseErroredResponse) {}

/**
 * @given RPCLibp2p
 * @when reading a message without waiting for a response
 * @then operation completes successfully
 */
TEST_F(RpcLibp2pTest, ReadWithoutResponse) {}

TEST_F(RpcLibp2pTest, WriteWithResponse) {}

TEST_F(RpcLibp2pTest, WriteWithResponseErroredWrite) {}

TEST_F(RpcLibp2pTest, WriteWithoutResponse) {}
