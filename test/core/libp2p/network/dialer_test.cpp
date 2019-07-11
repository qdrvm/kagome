/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include "libp2p/network/impl/dialer_impl.hpp"

#include "mock/libp2p/connection/capable_connection_mock.hpp"
#include "mock/libp2p/network/connection_manager_mock.hpp"
#include "mock/libp2p/network/router_mock.hpp"
#include "mock/libp2p/network/transport_manager_mock.hpp"
#include "mock/libp2p/peer/address_repository_mock.hpp"
#include "mock/libp2p/protocol_muxer/protocol_muxer_mock.hpp"
#include "mock/libp2p/transport/transport_mock.hpp"

#include "testutil/gmock_actions.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using namespace libp2p;
using namespace network;
using namespace connection;
using namespace protocol;
using namespace transport;
using namespace protocol_muxer;

using ::testing::_;
using ::testing::Return;

struct DialerTest : public ::testing::Test {
  std::shared_ptr<CapableConnectionMock> connection =
      std::make_shared<CapableConnectionMock>();

  std::shared_ptr<TransportMock> transport = std::make_shared<TransportMock>();

  std::shared_ptr<peer::AddressRepository> addrepo =
      std::make_shared<peer::AddressRepositoryMock>();

  std::shared_ptr<ProtocolMuxerMock> proto_muxer =
      std::make_shared<ProtocolMuxerMock>();

  std::shared_ptr<TransportManagerMock> tmgr =
      std::make_shared<TransportManagerMock>();

  std::shared_ptr<ConnectionManagerMock> cmgr =
      std::make_shared<ConnectionManagerMock>();

  std::shared_ptr<Dialer> dialer =
      std::make_shared<DialerImpl>(proto_muxer, tmgr, cmgr);

  multi::Multiaddress ma1 = "/ip4/127.0.0.1/tcp/1"_multiaddr;
  peer::PeerId pid = "1"_peerid;

  peer::PeerInfo pinfo{.id = pid, .addresses = {ma1}};
};

/**
 * @given no known connections to peer, have 1 transport, 1 address supplied
 * @when dial
 * @then create new connection using transport
 */
TEST_F(DialerTest, DialNewConnection) {
  // we dont have connection already
  EXPECT_CALL(*cmgr, getBestConnectionForPeer(pinfo.id))
      .WillOnce(Return(nullptr));

  // we have transport to dial
  EXPECT_CALL(*tmgr, findBest(ma1)).WillOnce(Return(transport));

  // transport->dial returns valid connection
  EXPECT_CALL(*transport, dial(pinfo.id, ma1, _))
      .WillOnce(Arg2CallbackWithArg(outcome::success(connection)));

  // connection is stored by connection manager
  EXPECT_CALL(*cmgr, addConnectionToPeer(pinfo.id, _)).Times(1);

  bool executed = false;
  dialer->dial(pinfo, [&](auto &&rconn) {
    EXPECT_OUTCOME_TRUE(conn, rconn);
    executed = true;
  });

  ASSERT_TRUE(executed);
}

/**
 * @given no known connections to peer, no addresses supplied
 * @when dial
 * @then create new connection using transport
 */
TEST_F(DialerTest, DialNoAddresses) {
  // we dont have connection already
  EXPECT_CALL(*cmgr, getBestConnectionForPeer(pinfo.id))
      .WillOnce(Return(nullptr));

  // no addresses supplied
  peer::PeerInfo pinfo = {.id = pid};
  bool executed = false;
  dialer->dial(pinfo, [&](auto &&rconn) {
    EXPECT_OUTCOME_FALSE(e, rconn);
    EXPECT_EQ(e.value(), (int)std::errc::destination_address_required);
    executed = true;
  });

  ASSERT_TRUE(executed);
}

/**
 * @given no known connections to peer, have 1 tcp transport, 1 UDP address supplied
 * @when dial
 * @then can not dial, no transports found
 */
TEST_F(DialerTest, DialNoTransports) {
  // we dont have connection already
  EXPECT_CALL(*cmgr, getBestConnectionForPeer(pinfo.id))
      .WillOnce(Return(nullptr));

  // we did not find transport to dial
  EXPECT_CALL(*tmgr, findBest(ma1)).WillOnce(Return(nullptr));

  bool executed = false;
  dialer->dial(pinfo, [&](auto &&rconn) {
    EXPECT_OUTCOME_FALSE(e, rconn);
    EXPECT_EQ(e.value(), (int)std::errc::address_family_not_supported);
    executed = true;
  });

  ASSERT_TRUE(executed);
}
