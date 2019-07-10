/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/protocol/identify/identify_delta.hpp"

#include <gtest/gtest.h>
#include "libp2p/multi/uvarint.hpp"
#include "libp2p/protocol/identify/pb/identify.pb.h"
#include "mock/libp2p/connection/capable_connection_mock.hpp"
#include "mock/libp2p/connection/stream_mock.hpp"
#include "mock/libp2p/host_mock.hpp"
#include "mock/libp2p/network/network_mock.hpp"
#include "mock/libp2p/peer/peer_repository_mock.hpp"
#include "testutil/gmock_actions.hpp"
#include "testutil/literals.hpp"

using namespace libp2p;
using namespace peer;
using namespace crypto;
using namespace protocol;
using namespace network;
using namespace connection;
using namespace kagome::common;
using namespace multi;

using testing::_;
using testing::Const;
using testing::Ref;
using testing::Return;
using testing::ReturnRef;

class IdentifyDeltaTest : public testing::Test {
 public:
  void SetUp() override {
    for (const auto &proto : added_protos_) {
      msg_added_protos_.mutable_delta()->add_added_protocols(proto);
      msg_added_rm_protos_.mutable_delta()->add_added_protocols(proto);
    }
    for (const auto &proto : removed_protos_) {
      msg_added_rm_protos_.mutable_delta()->add_rm_protocols(proto);
    }

    UVarint added_proto_len{
        static_cast<uint64_t>(msg_added_protos_.ByteSize())};
    msg_added_protos_bytes_.insert(
        msg_added_protos_bytes_.end(),
        std::make_move_iterator(added_proto_len.toVector().begin()),
        std::make_move_iterator(added_proto_len.toVector().end()));
    msg_added_protos_bytes_.insert(msg_added_protos_bytes_.end(),
                                   msg_added_protos_.ByteSize(), 0);
    msg_added_protos_.SerializeToArray(
        msg_added_protos_bytes_.data() + added_proto_len.size(),
        msg_added_protos_.ByteSize());

    UVarint added_rm_proto_len{
        static_cast<uint64_t>(msg_added_rm_protos_.ByteSize())};
    msg_added_rm_protos_bytes_.insert(
        msg_added_rm_protos_bytes_.end(),
        std::make_move_iterator(added_rm_proto_len.toVector().begin()),
        std::make_move_iterator(added_rm_proto_len.toVector().end()));
    msg_added_rm_protos_bytes_.insert(msg_added_rm_protos_bytes_.end(),
                                      msg_added_rm_protos_.ByteSize(), 0);
    msg_added_rm_protos_.SerializeToArray(
        msg_added_rm_protos_bytes_.data() + added_rm_proto_len.size(),
        msg_added_rm_protos_.ByteSize());
  }

  HostMock host_;
  libp2p::event::Bus bus_;

  std::shared_ptr<IdentifyDelta> id_delta_ =
      std::make_shared<IdentifyDelta>(static_cast<Host &>(host_), bus_);

  std::vector<peer::Protocol> added_protos_{"/ping/1.0.0", "/ping/1.5.0"};
  std::vector<peer::Protocol> removed_protos_{"/http/5.2.8"};

  identify::pb::Identify msg_added_protos_;
  std::vector<uint8_t> msg_added_protos_bytes_;

  identify::pb::Identify msg_added_rm_protos_;
  std::vector<uint8_t> msg_added_rm_protos_bytes_;

  NetworkMock network_;
  PeerRepositoryMock peer_repo_;
  std::shared_ptr<CapableConnectionMock> conn_ =
      std::make_shared<CapableConnectionMock>();
  std::shared_ptr<StreamMock> stream_ = std::make_shared<StreamMock>();

  const std::string kIdentifyDeltaProtocol = "/p2p/id/delta/1.0.0";
  const PeerId kRemotePeerId = "xxxMyPeerIdxxx"_peerid;
  const PeerInfo kPeerInfo{
      kRemotePeerId,
      std::vector<multi::Multiaddress>{"/ip4/12.34.56.78/tcp/123"_multiaddr,
                                       "/ip4/192.168.0.1"_multiaddr}};
};

ACTION_P(InvokeLambda, s) {
  arg2(std::move(s));
}

/**
 * @given Identify-Delta
 * @when new protocols event is arrived
 * @then an Identify-Delta message with those protocols is sent over the network
 */
TEST_F(IdentifyDeltaTest, Send) {
  // getActivePeers
  EXPECT_CALL(host_, getNetwork())
      .WillOnce(ReturnRef(static_cast<const Network &>(network_)));
  EXPECT_CALL(network_, getConnections())
      .WillOnce(Return(std::vector<std::shared_ptr<CapableConnection>>{conn_}));
  EXPECT_CALL(*conn_, remotePeer()).WillOnce(Return(kRemotePeerId));
  EXPECT_CALL(host_, getPeerRepository()).WillOnce(ReturnRef(peer_repo_));
  EXPECT_CALL(peer_repo_, getPeerInfo(kRemotePeerId))
      .WillOnce(Return(kPeerInfo));

  // stream handling and message sending
  EXPECT_CALL(host_, newStream(kPeerInfo, kIdentifyDeltaProtocol, _))
      .WillOnce(InvokeLambda(stream_));
  EXPECT_CALL(*stream_,
              write(gsl::span<const uint8_t>(msg_added_protos_bytes_),
                    msg_added_protos_bytes_.size(), _))
      .WillOnce(InvokeLambda(outcome::success()));

  id_delta_->start();
  bus_.getChannel<network::event::ProtocolsAddedChannel>().publish(
      added_protos_);
}

TEST_F(IdentifyDeltaTest, Receive) {}
