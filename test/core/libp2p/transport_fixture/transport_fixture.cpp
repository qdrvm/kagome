/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "core/libp2p/transport_fixture/transport_fixture.hpp"

#include "libp2p/muxer/yamux/yamux_frame.hpp"
#include "libp2p/transport/impl/transport_impl.hpp"
#include "libp2p/transport/muxed_connection.hpp"

namespace libp2p::testing {
  using kagome::common::Buffer;
  using libp2p::multi::Multiaddress;
  using libp2p::muxer::Yamux;
  using libp2p::muxer::YamuxConfig;
  using libp2p::stream::Stream;
  using libp2p::transport::Connection;
  using libp2p::transport::TransportImpl;

  using std::chrono_literals::operator""ms;

  void TransportFixture::SetUp() {
    init();
  }

  void TransportFixture::init() {
    // create transport
    transport_ = std::make_unique<TransportImpl>(context_);
    ASSERT_TRUE(transport_) << "cannot create transport";

    // create a listener, which is going to wrap new connections to Yamux
    transport_listener_ = transport_->createListener(
        [this](std::shared_ptr<Connection> c) mutable {  // NOLINT
          ASSERT_FALSE(c->isClosed());
          ASSERT_TRUE(c) << "createListener: connection is nullptr";

          // here, our Yamux instance is going to be a server, as a new
          // connection is accepted
          yamux_ = std::make_shared<Yamux>(
              std::move(c),
              [this](std::unique_ptr<Stream> new_stream) mutable {
                accepted_streams_.push_back(std::move(new_stream));
              },
              YamuxConfig{true});

          ASSERT_TRUE(yamux_) << "cannot create Yamux from a new connection";
          yamux_->start();
        });

    // create multiaddress, from which we are going to connect
    EXPECT_OUTCOME_TRUE(ma, Multiaddress::create("/ip4/127.0.0.1/tcp/40009"))
    EXPECT_TRUE(transport_listener_->listen(ma)) << "is port 40009 busy?";
    multiaddress_ = std::make_shared<Multiaddress>(std::move(ma));

    // dial to our "server", getting a connection
    EXPECT_OUTCOME_TRUE(conn, transport_->dial(*multiaddress_))
    connection_ = std::move(conn);

    // let Yamux be created
    context_.run_for(10ms);
    ASSERT_TRUE(yamux_);
  }

  void TransportFixture::launchContext() {
    context_.run_for(50ms);
  }

  void TransportFixture::checkIOSuccess(const std::error_code &ec,
                                        size_t received_size,
                                        size_t expected_size) {
    ASSERT_FALSE(ec);
    ASSERT_EQ(received_size, expected_size);  // NOLINT
  }

  std::unique_ptr<Stream> TransportFixture::getNewStream(
      Yamux::StreamId expected_stream_id) {
    auto stream = yamux_->newStream().value();

    auto new_stream_msg = muxer::newStreamMsg(expected_stream_id);
    auto rcvd_msg = std::make_shared<Buffer>(new_stream_msg.size(), 0);
    connection_->asyncRead(boost::asio::buffer(rcvd_msg->toVector()),
                           new_stream_msg.size(),
                           [&new_stream_msg, rcvd_msg](auto &&ec, auto &&n) {
                             checkIOSuccess(ec, n, new_stream_msg.size());
                             EXPECT_EQ(*rcvd_msg, new_stream_msg);  // NOLINT
                           });
    context_.run_for(10ms);

    return stream;
  }

}  // namespace libp2p::testing
