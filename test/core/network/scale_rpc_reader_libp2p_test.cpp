/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/scale_rpc_reader_libp2p.hpp"

#include <gtest/gtest.h>
#include "mock/libp2p/basic/read_writer_mock.hpp"
#include "network/types/block_response.hpp"
#include "scale/scale.hpp"

using namespace kagome;
using namespace network;

using namespace libp2p;
using namespace basic;

class ScaleRpcReaderLibp2pTest : public testing::Test {
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
 * @given ScaleRPCReader
 * @when reading a message @and answering with a response
 * @then operation completes successfully
 */
TEST_F(ScaleRpcReaderLibp2pTest, ReadWithResponse) {
  ScaleRPCLibp2p::read<BlockResponse, BlockResponse>(
      read_writer_,
      [this](auto &&received_request) {
        EXPECT_EQ(received_request.id, request.id);
        return response;
      },
      [](auto &&err) { FAIL() << err.error().message(); });
}

/**
 * @given ScaleRPCReader
 * @when reading a message @and answering with an error
 * @then that error is properly handled
 */
TEST_F(ScaleRpcReaderLibp2pTest, ReadWithResponseErroredResponse) {}

/**
 * @given ScaleRPCReader
 * @when reading a message without waiting for a response
 * @then operation completes successfully
 */
TEST_F(ScaleRpcReaderLibp2pTest, ReadWithoutResponse) {}
