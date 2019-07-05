/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/protocol/identify.hpp"

#include <gtest/gtest.h>
#include <libp2p/protocol/identify/pb/identify.pb.h>
#include "libp2p/network/network.hpp"
#include "mock/libp2p/connection/stream_mock.hpp"
#include "mock/libp2p/crypto/key_marshaller_mock.hpp"
#include "mock/libp2p/host_mock.hpp"
#include "mock/libp2p/peer/identity_manager_mock.hpp"
#include "testutil/literals.hpp"

using namespace libp2p;
using namespace peer;
using namespace crypto;
using namespace protocol;
using namespace network;
using namespace connection;
using namespace kagome::common;

using testing::_;

class IdentifyTest : public testing::Test {
 public:
  void SetUp() override {}

  std::shared_ptr<HostMock> host_ = std::make_shared<HostMock>();
  event::Bus bus_;
  IdentityManagerMock id_manager_;
  std::shared_ptr<marshaller::KeyMarshallerMock> key_marshaller_ =
      std::make_shared<marshaller::KeyMarshallerMock>();

  std::shared_ptr<Identify> identify_;

  std::shared_ptr<StreamMock> stream_;

  // Identify Protobuf message and its components
  identify::pb::Identify identify_pb_msg_;
  std::vector<Protocol> protocols_{"/http/5.0.1", "/dogeproto/2.2.8"};
  multi::Multiaddress remote_multiaddr_ = "/ip4/93.32.12.54/tcp/228"_multiaddr;
  std::vector<multi::Multiaddress> listen_addresses_{
      "/ip4/111.111.111.111/udp/21"_multiaddr,
      "/ip4/222.222.222.222/tcp/57"_multiaddr};
  Buffer marshalled_pubkey_{0x11, 0x22, 0x33, 0x44};
  PublicKey pubkey_{{Key::Type::RSA2048, marshalled_pubkey_}};
  static constexpr std::string_view kLibp2pVersion = "ipfs/0.1.0";
  static constexpr std::string_view kClientVersion = "cpp-libp2p/0.1.0";

  static constexpr std::string_view kIdentifyProto = "/ipfs/id/1.0.0";
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
