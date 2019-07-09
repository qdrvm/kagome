/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/protocol/identify.hpp"

#include <vector>

#include <gtest/gtest.h>
#include "libp2p/network/connection_manager.hpp"
#include "libp2p/network/network.hpp"
#include "libp2p/protocol/identify/pb/identify.pb.h"
#include "mock/libp2p/connection/capable_connection_mock.hpp"
#include "mock/libp2p/connection/stream_mock.hpp"
#include "mock/libp2p/crypto/key_marshaller_mock.hpp"
#include "mock/libp2p/host_mock.hpp"
#include "mock/libp2p/network/router_mock.hpp"
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
using testing::Const;
using testing::Ref;
using testing::Return;
using testing::ReturnRef;

class IdentifyTest : public testing::Test {
 public:
  void SetUp() override {
    // create a Protobuf message, which is to be "read" or written
    for (const auto &proto : protocols_) {
      identify_pb_msg_.add_protocols(proto);
    }
    identify_pb_msg_.set_observedaddr(
        std::string{remote_multiaddr_.getStringAddress()});
    for (const auto &addr : listen_addresses_) {
      identify_pb_msg_.add_listenaddrs(std::string{addr.getStringAddress()});
    }
    identify_pb_msg_.set_publickey(marshalled_pubkey_.data(),
                                   marshalled_pubkey_.size());
    identify_pb_msg_.set_protocolversion(kLibp2pVersion);
    identify_pb_msg_.set_agentversion(kClientVersion);

    identify_pb_msg_bytes_.insert(identify_pb_msg_bytes_.end(),
                                  identify_pb_msg_.ByteSize(), 0);
    identify_pb_msg_.SerializeToArray(identify_pb_msg_bytes_.data(),
                                      identify_pb_msg_.ByteSize());

    identify_ =
        std::make_shared<Identify>(host_, bus_, id_manager_, key_marshaller_);
  }

  std::shared_ptr<Host> host_ = std::make_shared<HostMock>();
  std::shared_ptr<HostMock> host_mock_ =
      std::static_pointer_cast<HostMock>(host_);
  libp2p::event::Bus bus_;
  IdentityManagerMock id_manager_;
  std::shared_ptr<marshaller::KeyMarshaller> key_marshaller_ =
      std::make_shared<marshaller::KeyMarshallerMock>();

  std::shared_ptr<Identify> identify_;

  std::shared_ptr<CapableConnectionMock> connection_ =
      std::make_shared<CapableConnectionMock>();
  std::shared_ptr<StreamMock> stream_ = std::make_shared<StreamMock>();

  // mocked host's components
  RouterMock router_;

  // Identify Protobuf message and its components
  identify::pb::Identify identify_pb_msg_;
  std::vector<uint8_t> identify_pb_msg_bytes_;

  std::vector<Protocol> protocols_{"/http/5.0.1", "/dogeproto/2.2.8"};
  multi::Multiaddress remote_multiaddr_ = "/ip4/93.32.12.54/tcp/228"_multiaddr;
  std::vector<multi::Multiaddress> listen_addresses_{
      "/ip4/111.111.111.111/udp/21"_multiaddr,
      "/ip4/222.222.222.222/tcp/57"_multiaddr};
  std::vector<uint8_t> marshalled_pubkey_{0x11, 0x22, 0x33, 0x44},
      pubkey_data_{0x55, 0x66, 0x77, 0x88};
  PublicKey pubkey_{{Key::Type::RSA2048, pubkey_data_}};
  KeyPair key_pair_{pubkey_, PrivateKey{}};
  const std::string kLibp2pVersion = "ipfs/0.1.0";
  const std::string kClientVersion = "cpp-libp2p/0.1.0";

  const peer::PeerId kRemotePeerId = "xxxMyCoolPeerxxx"_peerid;
  const peer::PeerInfo kPeerInfo{
      kRemotePeerId, std::vector<multi::Multiaddress>{remote_multiaddr_}};

  const std::string kIdentifyProto = "/ipfs/id/1.0.0";
};

ACTION_P2(Success, buf, res) {
  // better compare here, as this will show diff
  ASSERT_EQ(arg0, buf);
  ASSERT_EQ(arg1, buf.size());
  arg2(std::move(res));
}

ACTION_P(Close, res) {
  arg0(std::move(res));
}

/**
 * @given Identify object
 * @when a stream over Identify protocol is opened from another side
 * @then well-formed Identify message is sent by our peer
 */
TEST_F(IdentifyTest, Send) {
  // setup components, so that when Identify asks them, they give expected
  // parameters to be put into the Protobuf message
  EXPECT_CALL(*host_mock_, getRouter()).WillOnce(ReturnRef(Const(router_)));
  EXPECT_CALL(router_, getSupportedProtocols()).WillOnce(Return(protocols_));

  EXPECT_CALL(*stream_, remoteMultiaddr())
      .Times(2)
      .WillRepeatedly(Return(outcome::success(remote_multiaddr_)));

  EXPECT_CALL(*host_mock_, getListenAddresses())
      .WillOnce(Return(listen_addresses_));

  EXPECT_CALL(id_manager_, getKeyPair()).WillOnce(ReturnRef(Const(key_pair_)));
  EXPECT_CALL(
      *std::static_pointer_cast<marshaller::KeyMarshallerMock>(key_marshaller_),
      marshal(pubkey_))
      .WillOnce(Return(marshalled_pubkey_));

  EXPECT_CALL(*host_mock_, getLibp2pVersion()).WillOnce(Return(kLibp2pVersion));
  EXPECT_CALL(*host_mock_, getLibp2pClientVersion())
      .WillOnce(Return(kClientVersion));

  EXPECT_CALL(*stream_, remotePeerId()).WillOnce(Return(kRemotePeerId));

  // handle Identify request and check it
  EXPECT_CALL(*stream_, write(_, _, _))
      .WillOnce(Success(gsl::span<const uint8_t>(identify_pb_msg_bytes_.data(),
                                                 identify_pb_msg_bytes_.size()),
                        outcome::success(identify_pb_msg_bytes_.size())));
  identify_->handle(std::static_pointer_cast<Stream>(stream_));
}

ACTION_P(ReturnStream, s) {
  arg2(std::move(s));
}

/**
 * @given Identify object
 * @when a new connection event is triggered
 * @then Identify opens a new stream over that connection @and requests other
 * peer to be identified @and accepts the received message
 */
TEST_F(IdentifyTest, Receive) {
  EXPECT_CALL(*connection_, remotePeer()).WillOnce(Return(kRemotePeerId));
  EXPECT_CALL(*connection_, remoteMultiaddr)
      .WillOnce(Return(remote_multiaddr_));

  EXPECT_CALL(*host_mock_, newStream(kPeerInfo, kIdentifyProto, _))
      .WillOnce(ReturnStream(std::static_pointer_cast<Stream>(stream_)));

  // trigger the event, to which Identify object reacts
  identify_->start();
  bus_.getChannel<network::event::OnNewConnectionChannel>().publish(
      std::weak_ptr<CapableConnection>(connection_));
}
