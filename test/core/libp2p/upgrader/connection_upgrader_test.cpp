/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/upgrader/impl/connection_upgrader_impl.hpp"

#include <gtest/gtest.h>

#include "common/logger.hpp"
#include "libp2p/multi/multiaddress.hpp"
#include "libp2p/muxer/yamux/yamux_frame.hpp"
#include "libp2p/muxer/yamux/yamux_stream.hpp"
#include "libp2p/transport/all.hpp"
#include "testutil/outcome.hpp"

using kagome::common::Logger;
using libp2p::multi::Multiaddress;
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
  Logger logger_ = kagome::common::createLogger("ConnectionUpgraderTest");
  boost::asio::io_context context_;
  std::unique_ptr<Transport> transport_;
  std::shared_ptr<TransportListener> transport_listener_;
  std::shared_ptr<ConnectionUpgraderImpl> upgrader_ =
      std::make_shared<ConnectionUpgraderImpl>();
  std::unique_ptr<MuxedConnection> server_muxed_connection_;
  std::unique_ptr<MuxedConnection> client_muxed_connection_;
};

/**
 * @given transport, listener, context and local client prepared
 * @when context_.run_for() is called
 * @then upgrader_ receives and upgrades incoming connection to muxed
 */
TEST_F(ConnectionUpgraderTest, IntegrationTest) {
  // create transport
  transport_ = std::make_unique<TransportImpl>(context_);
  ASSERT_TRUE(transport_) << "cannot create transport";

  // create a listener, which is going to wrap new connections to YamuxedConnection
  transport_listener_ = transport_->createListener(
      [this](std::shared_ptr<Connection> server_connection) mutable {
        ASSERT_FALSE(server_connection->isClosed());
        ASSERT_TRUE(server_connection)
            << "createListener: connection is nullptr";

        // here, our YamuxedConnection instance is going to be a server, as a new
        // connection is accepted
        MuxerOptions server_options = {ConnectionType::SERVER_SIDE};
        server_muxed_connection_ = upgrader_->upgradeToMuxed(
            server_connection, server_options,
            [this](std::unique_ptr<Stream> new_stream) mutable {
              logger_->info("server muxed stream received");
            });
      });

  // create multiaddress, from which we are going to connect
  EXPECT_OUTCOME_TRUE(ma, Multiaddress::create("/ip4/127.0.0.1/tcp/40009"));
  ASSERT_TRUE(transport_listener_->listen(ma)) << "is port 40009 busy?";

  // dial to our "server", getting a connection
  EXPECT_OUTCOME_TRUE(client_connection, transport_->dial(ma));
  MuxerOptions client_options = {ConnectionType::CLIENT_SIDE};
  client_muxed_connection_ = upgrader_->upgradeToMuxed(
      client_connection, client_options,
      [this](std::unique_ptr<Stream> new_stream) mutable {
        logger_->info("client muxed stream received");
      });

  // let MuxedConnection be created
  context_.run_for(10ms);
  ASSERT_TRUE(server_muxed_connection_)
      << "failed to upgrade raw server connection to muxed";
  ASSERT_TRUE(client_muxed_connection_)
      << "failed to upgrade raw client connection to muxed";
}
