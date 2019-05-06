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

  std::shared_ptr<Yamux> yamux_;
  std::vector<std::unique_ptr<Stream>> accepted_streams_;

  std::unique_ptr<Transport> transport_;
  std::shared_ptr<TransportListener> transport_listener_;
  std::shared_ptr<Multiaddress> multiaddress_;
  std::shared_ptr<Connection> connection_;
  std::shared_ptr<ConnectionUpgraderImpl> upgrader_;
  std::unique_ptr<MuxedConnection> muxed_connection_;
};

/**
 * @given
 * @when
 * @then
 */
TEST_F(ConnectionUpgraderTest, ServerInstantiateSuccess) {
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
        muxed_connection_ = upgrader_->upgradeToMuxed(
            c, {ConnectionType::SERVER_SIDE},
            [this](std::unique_ptr<Stream> new_stream) mutable {
              accepted_streams_.push_back(std::move(new_stream));
              std::cout << "accepted new stream" << std::endl;
            });

        ASSERT_TRUE(yamux_) << "cannot create Yamux from a new connection";
        yamux_->start();
      });

  // create multiaddress, from which we are going to connect
  EXPECT_OUTCOME_TRUE(ma, Multiaddress::create("/ip4/127.0.0.1/tcp/40009"))
  EXPECT_TRUE(transport_listener_->listen(ma)) << "is port 40019 busy?";
  multiaddress_ = std::make_shared<Multiaddress>(std::move(ma));

  // dial to our "server", getting a connection
  EXPECT_OUTCOME_TRUE(conn, transport_->dial(*multiaddress_))
  connection_ = std::move(conn);

  // let MuxedConnection be created
  context_.run_for(100ms);
  ASSERT_TRUE(muxed_connection_);
}
