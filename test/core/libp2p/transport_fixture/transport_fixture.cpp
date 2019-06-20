/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "core/libp2p/transport_fixture/transport_fixture.hpp"

#include "libp2p/transport/tcp.hpp"
#include "mock/libp2p/connection/capable_connection_mock.hpp"
#include "testutil/literals.hpp"
#include "testutil/gmock_actions.hpp"

namespace {
  template <typename Conn, typename R, typename Callback>
  void _upgrade(Conn c, Callback cb) {
    R r =
        std::make_shared<libp2p::connection::CapableConnBasedOnRawConnMock>(c);
    cb(std::move(r));
  }
}  // namespace

namespace libp2p::testing {
  using kagome::common::Buffer;
  using libp2p::multi::Multiaddress;
  using libp2p::transport::TcpTransport;
  using libp2p::transport::Upgrader;
  using libp2p::transport::UpgraderMock;
  using ::testing::_;
  using ::testing::Invoke;
  using ::testing::NiceMock;
  using transport::TransportListener;

  auto TransportFixture::makeUpgrader() {
    auto upgrader = std::make_shared<NiceMock<UpgraderMock>>();
    ON_CALL(*upgrader, upgradeToSecure(_, _))
        .WillByDefault(Invoke(_upgrade<Upgrader::RawSPtr, Upgrader::SecureSPtr,
                                       Upgrader::OnSecuredCallbackFunc>));
    ON_CALL(*upgrader, upgradeToMuxed(_, _))
        .WillByDefault(
            Invoke(_upgrade<Upgrader::SecureSPtr, Upgrader::CapableSPtr,
                            Upgrader::OnMuxedCallbackFunc>));

    return upgrader;
  }

  TransportFixture::TransportFixture() : context_{1} {}

  void TransportFixture::SetUp() {
    transport_ = std::make_shared<TcpTransport>(context_, makeUpgrader());
    ASSERT_TRUE(transport_) << "cannot create transport1";

    // create multiaddress, from which we are going to connect
    auto ma = "/ip4/127.0.0.1/tcp/40009"_multiaddr;
    multiaddress_ = std::make_shared<Multiaddress>(std::move(ma));
  }

  void TransportFixture::server(const TransportListener::HandlerFunc &handler) {
    transport_listener_ = transport_->createListener(handler);
    EXPECT_TRUE(transport_listener_->listen(*multiaddress_))
        << "is port 40009 busy?";
  }

  void TransportFixture::client(const TransportListener::HandlerFunc &handler) {
    transport_->dial(*multiaddress_, handler);
  }

  void TransportFixture::launchContext() {
    using std::chrono_literals::operator""ms;
    context_.run_for(100ms);
  }

}  // namespace libp2p::testing
