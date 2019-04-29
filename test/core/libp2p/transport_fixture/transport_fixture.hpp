/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TRANSPORT_FIXTURE_HPP
#define KAGOME_TRANSPORT_FIXTURE_HPP

#include <gtest/gtest.h>
#include <testutil/outcome.hpp>
#include "libp2p/muxer/yamux.hpp"
#include "libp2p/muxer/yamux/yamux_frame.hpp"
#include "libp2p/transport/impl/transport_impl.hpp"
#include "libp2p/transport/muxed_connection.hpp"

namespace libp2p::testing {
  using namespace libp2p::transport;
  using namespace libp2p::multi;
  using namespace libp2p::stream;
  using namespace libp2p::muxer;
  using kagome::common::Buffer;

  using std::chrono_literals::operator""ms;

  /**
   * Support class, allowing to have a Yamuxed preset TCP connection
   */
  class TransportFixture : public ::testing::Test {
   public:
    void SetUp() override {
      init();
    }

    /**
     * Initialize:
     *    - transport
     *    - listener, which wraps new connections into Yamux
     *    - multiaddress, which is listened by the created transport
     *    - Yamux itself
     */
    void init() {
      // create transport
      transport_ = std::make_unique<TransportImpl>(context_);
      ASSERT_TRUE(transport_) << "cannot create transport";

      // create a listener, which is going to wrap new connections to Yamux
      transport_listener_ = transport_->createListener(
          [this](std::shared_ptr<Connection> c) mutable {
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

    /**
     * Run the context for some time to execute async operations
     */
    void launchContext() {
      context_.run_for(50ms);
    }

    static void checkIOSuccess(const std::error_code ec, size_t received_size,
                               size_t expected_size) {
      ASSERT_FALSE(ec);
      ASSERT_EQ(received_size, expected_size);
    }

    /**
     * Get a pointer to a new stream
     * @param expected_stream_id - id, which is expected to be assigned to that
     * stream
     * @return pointer to the stream
     */
    std::unique_ptr<Stream> getNewStream(
        Yamux::StreamId expected_stream_id = kDefaulExpectedStreamId) {
      auto stream = yamux_->newStream().value();

      auto new_stream_msg = newStreamMsg(expected_stream_id);
      auto rcvd_msg = std::make_shared<Buffer>(new_stream_msg.size(), 0);
      connection_->asyncRead(boost::asio::buffer(rcvd_msg->toVector()),
                             new_stream_msg.size(),
                             [&new_stream_msg, rcvd_msg](auto &&ec, auto &&n) {
                               checkIOSuccess(ec, n, new_stream_msg.size());
                               EXPECT_EQ(*rcvd_msg, new_stream_msg);
                             });
      context_.run_for(10ms);

      return stream;
    }

    boost::asio::io_context context_;

    std::shared_ptr<Yamux> yamux_;
    std::vector<std::unique_ptr<Stream>> accepted_streams_;

    std::unique_ptr<Transport> transport_;
    std::shared_ptr<TransportListener> transport_listener_;
    std::shared_ptr<Multiaddress> multiaddress_;
    std::shared_ptr<Connection> connection_;

    static constexpr Yamux::StreamId kDefaulExpectedStreamId = 2;
  };
}  // namespace libp2p::testing

#endif  // KAGOME_TRANSPORT_FIXTURE_HPP
