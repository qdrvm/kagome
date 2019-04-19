/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/muxer/yamux/yamux.hpp"

#include <gtest/gtest.h>
#include "libp2p/multi/multiaddress.hpp"
#include "libp2p/muxer/yamux/yamux_frame.hpp"
#include "libp2p/muxer/yamux/yamux_stream.hpp"
#include "libp2p/transport/all.hpp"
#include "testutil/outcome.hpp"

using namespace libp2p::muxer;
using namespace libp2p::transport;
using namespace libp2p::stream;
using namespace kagome::common;
using namespace libp2p::multi;

using std::chrono_literals::operator""ms;

class YamuxIntegrationTest
    : public ::testing::Test,
      public std::enable_shared_from_this<YamuxIntegrationTest> {
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
    context_.run_for(1000ms);
  }

  static void checkIOSuccess(const std::error_code ec, size_t received_size,
                             size_t expected_size) {
    ASSERT_FALSE(ec);
    ASSERT_EQ(received_size, expected_size);
  }

  /**
   * Get a pointer to a new stream
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

  std::shared_ptr<Yamux> yamux_;
  std::vector<std::unique_ptr<Stream>> accepted_streams_;

  boost::asio::io_context context_;
  std::unique_ptr<Transport> transport_;
  std::shared_ptr<TransportListener> transport_listener_;
  std::shared_ptr<Multiaddress> multiaddress_;
  std::shared_ptr<Connection> connection_;

  static constexpr Yamux::StreamId kDefaulExpectedStreamId = 2;
};

/**
 * @given initialized Yamux
 * @when creating a new stream from the client's side
 * @then stream is created @and corresponding ack message is sent to the client
 */
TEST_F(YamuxIntegrationTest, StreamFromClient) {
  constexpr Yamux::StreamId created_stream_id = 1;

  auto new_stream_ack_msg_rcv =
      std::make_shared<Buffer>(YamuxFrame::kHeaderLength, 0);
  auto new_stream_msg = newStreamMsg(created_stream_id);

  connection_->asyncWrite(
      boost::asio::buffer(new_stream_msg.toVector()),
      [this, &new_stream_msg, new_stream_ack_msg_rcv, created_stream_id](
          auto &&ec, auto &&n) {
        checkIOSuccess(ec, n, new_stream_msg.size());

        connection_->asyncRead(
            boost::asio::buffer(new_stream_ack_msg_rcv->toVector()),
            YamuxFrame::kHeaderLength,
            [this, new_stream_ack_msg_rcv, created_stream_id](auto &&ec,
                                                              auto &&n) {
              // check a new stream is in our 'accepted_streams'
              ASSERT_EQ(accepted_streams_.size(), 1);

              checkIOSuccess(ec, n, YamuxFrame::kHeaderLength);

              // check our yamux has sent an ack message for that
              // stream
              auto parsed_ack_opt =
                  parseFrame(new_stream_ack_msg_rcv->toVector());
              ASSERT_TRUE(parsed_ack_opt);
              ASSERT_EQ(parsed_ack_opt->stream_id_, created_stream_id);
            });
      });

  launchContext();
}

/**
 * @given initialized Yamux
 * @when creating a new stream from the server's side
 * @then stream is created @and corresponding new stream message is received by
 * the client
 */
TEST_F(YamuxIntegrationTest, StreamFromServer) {
  constexpr Yamux::StreamId expected_stream_id = 2;

  auto stream_res = yamux_->newStream();
  ASSERT_TRUE(stream_res);

  auto stream = std::move(stream_res.value());
  ASSERT_FALSE(stream->isClosedForRead());
  ASSERT_FALSE(stream->isClosedForWrite());
  ASSERT_FALSE(stream->isClosedEntirely());

  auto expected_new_stream_msg = newStreamMsg(expected_stream_id);
  auto new_stream_msg_buf =
      std::make_shared<Buffer>(YamuxFrame::kHeaderLength, 0);
  connection_->asyncRead(
      boost::asio::buffer(new_stream_msg_buf->toVector()),
      YamuxFrame::kHeaderLength,
      [new_stream_msg_buf, &expected_new_stream_msg](auto &&ec, auto &&n) {
        checkIOSuccess(ec, n, YamuxFrame::kHeaderLength);

        ASSERT_EQ(*new_stream_msg_buf, expected_new_stream_msg);
      });

  launchContext();
}

/**
 * @given initialized Yamux @and streams, multiplexed by that Yamux
 * @When writing to that stream
 * @then the operation is succesfully executed
 */
TEST_F(YamuxIntegrationTest, StreamWrite) {
  auto stream = getNewStream();

  Buffer data{{0x12, 0x34, 0xAA}};
  auto expected_data_msg = dataMsg(kDefaulExpectedStreamId, data);
  auto received_data_msg =
      std::make_shared<Buffer>(expected_data_msg.size(), 0);

  stream->writeAsync(
      data, [this, &expected_data_msg, received_data_msg](auto &&ec, auto &&n) {
        checkIOSuccess(ec, n, expected_data_msg.size());

        // check that our written data has achieved the
        // destination
        connection_->asyncRead(
            boost::asio::buffer(received_data_msg->toVector()),
            expected_data_msg.size(),
            [&expected_data_msg, received_data_msg](auto &&ec, auto &&n) {
              checkIOSuccess(ec, n, expected_data_msg.size());

              ASSERT_EQ(*received_data_msg, expected_data_msg);
            });
      });

  launchContext();
}

/**
 * @given initialized Yamux @and streams, multiplexed by that Yamux
 * @when reading from that stream
 * @then the operation is succesfully executed
 */
TEST_F(YamuxIntegrationTest, StreamRead) {
  auto stream = getNewStream();

  Buffer data{{0x12, 0x34, 0xAA}};
  auto written_data_msg = dataMsg(kDefaulExpectedStreamId, data);

  connection_->asyncWrite(
      boost::asio::buffer(written_data_msg.toVector()),
      [&written_data_msg, &stream, &data](auto &&ec, auto &&n) mutable {
        checkIOSuccess(ec, n, written_data_msg.size());

        stream->readAsync([&data](Stream::NetworkMessageOutcome msg_res) {
          ASSERT_TRUE(msg_res);
          ASSERT_EQ(msg_res.value(), data);
        });
      });

  launchContext();
}

/**
 * @given initialized Yamux
 * @when a ping message arrives to Yamux
 * @then Yamux sends a ping response back
 */
TEST_F(YamuxIntegrationTest, Ping) {
  constexpr uint32_t ping_value = 42;

  auto ping_in_msg = pingOutMsg(ping_value);
  auto ping_out_msg = pingResponseMsg(ping_value);
  auto received_ping = std::make_shared<Buffer>(ping_out_msg.size(), 0);

  std::cout << "We have reached 1" << std::endl;
  connection_->asyncWrite(
      boost::asio::buffer(ping_in_msg.toVector()),
      [this, &ping_in_msg, &ping_out_msg, received_ping](auto &&ec, auto &&n) {
        std::cout << "We have reached 2" << std::endl;
        checkIOSuccess(ec, n, ping_in_msg.size());
        std::cout << "We have reached 3" << std::endl;

        connection_->asyncRead(
            boost::asio::buffer(received_ping->toVector()), ping_out_msg.size(),
            [this, &ping_out_msg, received_ping](auto &&ec, auto &&n) {
              std::cout << "We have reached 4" << std::endl;
              checkIOSuccess(ec, n, ping_out_msg.size());
              std::cout << "We have reached 5" << std::endl;

              ASSERT_EQ(*received_ping, ping_out_msg);
              std::cout << "We have reached 6" << std::endl;
            });
      });

  launchContext();
  std::cout << "We have reached 7" << std::endl;
}

/**
 * @given initialized Yamux @and stream over it
 * @when closing that stream for writes
 * @then the stream is closed for writes @and corresponding message is received
 * on the other side
 */
TEST_F(YamuxIntegrationTest, CloseForWrites) {
  auto stream = getNewStream();

  ASSERT_FALSE(stream->isClosedForWrite());
  stream->close();
  ASSERT_TRUE(stream->isClosedForWrite());

  auto expected_close_stream_msg = closeStreamMsg(kDefaulExpectedStreamId);
  auto close_stream_msg_rcv =
      std::make_shared<Buffer>(YamuxFrame::kHeaderLength, 0);

  connection_->asyncRead(
      boost::asio::buffer(close_stream_msg_rcv->toVector()),
      YamuxFrame::kHeaderLength,
      [&expected_close_stream_msg, close_stream_msg_rcv](auto &&ec, auto &&n) {
        checkIOSuccess(ec, n, YamuxFrame::kHeaderLength);

        ASSERT_EQ(*close_stream_msg_rcv, expected_close_stream_msg);
      });

  launchContext();
}

/**
 * @given initialized Yamux @and stream over it
 * @when the other side sends a close message for that stream
 * @then the stream is closed for reads
 */
TEST_F(YamuxIntegrationTest, CloseForReads) {
  auto stream = getNewStream();

  ASSERT_FALSE(stream->isClosedForRead());

  auto sent_close_stream_msg = closeStreamMsg(kDefaulExpectedStreamId);

  connection_->asyncWrite(
      boost::asio::buffer(sent_close_stream_msg.toVector()),
      [&sent_close_stream_msg, &stream](auto &&ec, auto &&n) {
        checkIOSuccess(ec, n, sent_close_stream_msg.size());
      });

  launchContext();
  ASSERT_TRUE(stream->isClosedForRead());
}

/**
 * @given initialized Yamux @and stream over it
 * @when close message is sent over the stream @and the other side responses
 * with a close message as well
 * @then the stream is closed entirely - removed from Yamux
 */
TEST_F(YamuxIntegrationTest, CloseEntirely) {
  auto stream = getNewStream();

  ASSERT_FALSE(stream->isClosedForWrite());
  stream->close();
  ASSERT_TRUE(stream->isClosedForWrite());

  auto expected_close_stream_msg = closeStreamMsg(kDefaulExpectedStreamId);
  auto close_stream_msg_rcv =
      std::make_shared<Buffer>(YamuxFrame::kHeaderLength, 0);

  connection_->asyncRead(
      boost::asio::buffer(close_stream_msg_rcv->toVector()),
      YamuxFrame::kHeaderLength,
      [this, &stream, &expected_close_stream_msg, close_stream_msg_rcv](
          auto &&ec, auto &&n) {
        checkIOSuccess(ec, n, YamuxFrame::kHeaderLength);
        ASSERT_EQ(*close_stream_msg_rcv, expected_close_stream_msg);

        connection_->asyncWrite(
            boost::asio::buffer(expected_close_stream_msg.toVector()),
            [&expected_close_stream_msg, &stream](auto &&ec, auto &&n) {
              checkIOSuccess(ec, n, expected_close_stream_msg.size());
            });
      });

  launchContext();
  ASSERT_TRUE(stream->isClosedEntirely());
}

/**
 * @given initialized Yamux @and stream over it
 * @when a reset message is sent over that stream
 * @then the stream is closed entirely - removed from Yamux @and the other side
 * receives a corresponding message
 */
TEST_F(YamuxIntegrationTest, Reset) {
  auto stream = getNewStream();

  ASSERT_FALSE(stream->isClosedEntirely());
  stream->reset();
  ASSERT_TRUE(stream->isClosedEntirely());

  auto expected_reset_msg = resetStreamMsg(kDefaulExpectedStreamId);
  auto rcvd_msg = std::make_shared<Buffer>(expected_reset_msg.size(), 0);

  connection_->asyncRead(boost::asio::buffer(rcvd_msg->toVector()),
                         expected_reset_msg.size(),
                         [&expected_reset_msg, rcvd_msg](auto &&ec, auto &&n) {
                           checkIOSuccess(ec, n, rcvd_msg->size());
                           ASSERT_EQ(*rcvd_msg, expected_reset_msg);
                         });

  launchContext();
}

/**
 * @given initialized Yamux
 * @when Yamux is closed
 * @then an underlying connection is closed @and the other side receives a
 * corresponding message
 */
TEST_F(YamuxIntegrationTest, GoAway) {
  yamux_->close();
  ASSERT_TRUE(yamux_->isClosed());
}
