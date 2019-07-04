/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/protocol/identify.hpp"

#include <gtest/gtest.h>
#include "libp2p/network/network.hpp"
#include "mock/libp2p/connection/stream_mock.hpp"
#include "mock/libp2p/crypto/key_marshaller_mock.hpp"
#include "mock/libp2p/host_mock.hpp"
#include "mock/libp2p/peer/identity_manager_mock.hpp"

using namespace libp2p;
using namespace peer;
using namespace crypto;
using namespace protocol;
using namespace network;
using namespace connection;

using testing::_;

class IdentifyTest : public testing::Test {
 public:
  std::shared_ptr<HostMock> host_ = std::make_shared<HostMock>();
  event::Bus bus_;
  IdentityManagerMock id_manager_;
  std::shared_ptr<marshaller::KeyMarshallerMock> key_marshaller_ =
      std::make_shared<marshaller::KeyMarshallerMock>();

  std::shared_ptr<Identify> identify_;

  std::shared_ptr<StreamMock> stream_;

  const std::string kIdentifyProto = "/ipfs/id/1.0.0";
};

ACTION_P(SaveLambda, l) {
  l = std::move(arg1);
}

/**
 * @given Identify object
 * @when a stream over Identify protocol is opened from another side
 * @then well-formed Identify message is sent by our peer
 */
TEST_F(IdentifyTest, Send) {
  // save lambda, which is to be called with a new stream over Identify protocol
  std::function<connection::Stream::Handler> new_stream_lambda;
  EXPECT_CALL(*host_, setProtocolHandler(kIdentifyProto, _))
      .WillOnce(SaveLambda(new_stream_lambda));

  identify_ =
      std::make_shared<Identify>(host_, bus_, id_manager_, key_marshaller_);

  // create a Protobuf message of expected components, wrap calls to different
  // functions to EXPECT_CALLs and check the two buffers
}

TEST_F(IdentifyTest, Receive) {}
