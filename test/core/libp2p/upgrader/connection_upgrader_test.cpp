/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/upgrader/impl/connection_upgrader_impl.hpp"

#include <gtest/gtest.h>

#include "libp2p/multi/multiaddress.hpp"
#include "libp2p/muxer/yamux/yamux_frame.hpp"
#include "libp2p/muxer/yamux/yamux_stream.hpp"
#include "libp2p/transport/all.hpp"
#include "testutil/outcome.hpp"

using libp2p::multi::Multiaddress;
using libp2p::muxer::Yamux;
using libp2p::muxer::YamuxConfig;
using libp2p::muxer::YamuxFrame;
using libp2p::stream::Stream;
using libp2p::transport::Connection;
using libp2p::transport::MuxedConnection;
using libp2p::transport::Transport;
using libp2p::transport::TransportImpl;
using libp2p::transport::TransportListener;
using libp2p::upgrader::ConnectionType;
using libp2p::upgrader::ConnectionUpgraderImpl;
using libp2p::upgrader::MuxerOptions;

using std::chrono_literals::operator""ms;

class ConnectionUpgraderTest : public ::testing::Test {
 protected:
  boost::asio::io_context context_;

  std::unique_ptr<Transport> transport_;
  std::shared_ptr<TransportListener> transport_listener_;
  std::shared_ptr<Multiaddress> multiaddress_;
  std::shared_ptr<ConnectionUpgraderImpl> upgrader_ =
      std::make_shared<ConnectionUpgraderImpl>();
  std::unique_ptr<MuxedConnection> muxed_connection_;
};

/**
 * @given transport, listener, context and local client
 * @when context_.run_for() is called
 * @then upgrader_ upgrades incoming connection to muxed
 */
TEST_F(ConnectionUpgraderTest, ServerUpgradeSuccess) {
  // create transport
  transport_ = std::make_unique<TransportImpl>(context_);
  ASSERT_TRUE(transport_) << "cannot create transport";

  // create a listener, which is going to wrap new connections to Yamux
  transport_listener_ =
      transport_->createListener([this](std::shared_ptr<Connection> c) mutable {
        ASSERT_FALSE(c->isClosed());
        ASSERT_TRUE(c) << "createListener: connection is nullptr";

        // here, our Yamux instance is going to be a server, as a new
        // connection is accepted
        MuxerOptions options = {ConnectionType::SERVER_SIDE};
        muxed_connection_ = upgrader_->upgradeToMuxed(
            c, options, [](std::unique_ptr<Stream> new_stream) mutable {
              // do nothing
            });
      });

  // create multiaddress, from which we are going to connect
  auto &&ma_res = Multiaddress::create("/ip4/127.0.0.1/tcp/40009");
  ASSERT_TRUE(ma_res);
  auto &&ma = ma_res.value();
  ASSERT_TRUE(transport_listener_->listen(ma)) << "is port 40009 busy?";
  multiaddress_ = std::make_shared<Multiaddress>(std::move(ma));

  // dial to our "server", getting a connection
  EXPECT_OUTCOME_TRUE_void(conn, transport_->dial(*multiaddress_))

  // let MuxedConnection be created
  context_.run_for(10ms);
  ASSERT_TRUE(muxed_connection_) << "failed to upgrade raw connection to muxed";
}

/**
 * @given
 * @when
 * @then
 */
TEST_F(ConnectionUpgraderTest, DISABLED_ClientUpgradeSuccess) {
  FAIL() << "not implemented yet";
}
