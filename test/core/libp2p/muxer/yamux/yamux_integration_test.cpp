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

class YamuxIntegrationTest : public ::testing::Test {
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

    // wait until yamux gets created from that connection
    context_.run_for(10ms);
    ASSERT_TRUE(yamux_);
  }

  /**
   * Run the context for some time to execute async operations
   */
  void launchContext() {
    context_.run_for(1000ms);
  }

  /**
   * Get a pointer to a new stream
   * @return pointer to the stream
   */
  std::unique_ptr<Stream> getNewStream() {
    return yamux_->newStream().value();
  }

  std::shared_ptr<Yamux> yamux_;
  std::vector<std::unique_ptr<Stream>> accepted_streams_;

  boost::asio::io_context context_;
  std::unique_ptr<Transport> transport_;
  std::shared_ptr<TransportListener> transport_listener_;
  std::shared_ptr<Multiaddress> multiaddress_;
  std::shared_ptr<Connection> connection_;

  std::function<libp2p::basic::Readable::CompletionHandler>
      empty_completion_handler = [](auto &&, auto &&) {};
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
      [this, new_stream_ack_msg_rcv, created_stream_id](auto &&ec, auto &&n) {
        ASSERT_FALSE(ec);
        connection_->asyncRead(
            boost::asio::buffer(new_stream_ack_msg_rcv->toVector()),
            YamuxFrame::kHeaderLength,
            [this, new_stream_ack_msg_rcv, created_stream_id](auto &&ec,
                                                              auto &&n) {
              // check a new stream is in our 'accepted_streams'
              ASSERT_EQ(accepted_streams_.size(), 1);

              ASSERT_FALSE(ec);
              ASSERT_EQ(n, YamuxFrame::kHeaderLength);

              // check our yamux has sent an ack message for that
              // stream
              auto parsed_ack_opt =
                  parseFrame(new_stream_ack_msg_rcv->toVector());
              ASSERT_TRUE(parsed_ack_opt);
              ASSERT_EQ(parsed_ack_opt->stream_id_, created_stream_id);
            });
      });

  launchContext();

  // read from that stream
  // check the read message
  // write to that stream
  // check that our message has come

  // read from that stream - must be a ping
  // response with out ping
  // check that our response worked

  // read from that stream - must be data
  // check that data
  // write some data back
  // check that the data is received

  // close stream for writes from this side
  // check it's closed for write on this side
  // check it's closed for read on the other side

  // read from that stream - must be a close-for-reads message from the other
  // side check stream is closed on both sides

  // create a stream from this side
  // check it's created on both sides

  // send go away message
  // check connection is broken on both sides
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
      [&new_stream_msg_buf, &expected_new_stream_msg](auto &&ec, auto &&n) {
        ASSERT_FALSE(ec);
        ASSERT_EQ(n, YamuxFrame::kHeaderLength);

        ASSERT_EQ(*new_stream_msg_buf, expected_new_stream_msg);
      });

  launchContext();
}

/**
 * @given initialized Yamux @and stream, multiplexed by that Yamux
 * @When writing to that stream
 * @then the operation is succesfully executed
 */
TEST_F(YamuxIntegrationTest, StreamWrite) {
  constexpr Yamux::StreamId expected_stream_id = 2;
  auto stream = getNewStream();

  Buffer data{{0x12, 0x34, 0xAA}};
  auto expected_data_msg = dataMsg(expected_stream_id, data);
  auto received_data_msg =
      std::make_shared<Buffer>(expected_data_msg.size(), 0);

  stream->writeAsync(
      data, [this, &expected_data_msg, received_data_msg](auto &&ec, auto &&n) {
        ASSERT_FALSE(ec);
        ASSERT_EQ(n, expected_data_msg.size());

        // check that our written data has achieved the
        // destination
        connection_->asyncRead(
            boost::asio::buffer(received_data_msg->toVector()),
            expected_data_msg.size(),
            [&expected_data_msg, received_data_msg](auto &&ec, auto &&n) {
              ASSERT_FALSE(ec);
              ASSERT_EQ(n, expected_data_msg.size());

              ASSERT_EQ(*received_data_msg, expected_data_msg);
            });
      });

  launchContext();
}

/**
 * @given initialized Yamux @and stream, multiplexed by that Yamux
 * @When reading from that stream
 * @then the operation is succesfully executed
 */
TEST_F(YamuxIntegrationTest, StreamRead) {
  constexpr Yamux::StreamId expected_stream_id = 2;
  auto stream = getNewStream();

  Buffer data{{0x12, 0x34, 0xAA}};
  auto written_data_msg = dataMsg(expected_stream_id, data);

  connection_->asyncWrite(
      boost::asio::buffer(written_data_msg.toVector()),
      [this, &written_data_msg, &stream, &data](auto &&ec, auto &&n) mutable {
        ASSERT_FALSE(ec);
        ASSERT_EQ(n, written_data_msg.size());

        stream->readAsync([&data](Stream::NetworkMessageOutcome msg_res) {
          ASSERT_TRUE(msg_res);
          ASSERT_EQ(msg_res.value(), data);
        });
      });

  launchContext();
}
