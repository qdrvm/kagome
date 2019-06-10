/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/connection/yamux/yamuxed_connection.hpp"

#include <future>

#include <gtest/gtest.h>
#include <testutil/outcome.hpp>
#include "core/libp2p/transport_fixture/transport_fixture.hpp"
#include "libp2p/connection/raw_connection.hpp"
#include "libp2p/connection/yamux/yamux_frame.hpp"
#include "libp2p/connection/yamux/yamux_stream.hpp"
#include "libp2p/multi/multiaddress.hpp"
#include "libp2p/muxer/yamux/yamux.hpp"
#include "libp2p/security/plaintext.hpp"

using namespace libp2p::transport;
using namespace kagome::common;
using namespace libp2p::multi;
using namespace libp2p::connection;
using namespace libp2p::security;
using namespace libp2p::muxer;

using libp2p::basic::ReadWriteCloser;
using libp2p::testing::TransportFixture;
using std::chrono_literals::operator""ms;

#define CLIENT_FAIL_LAMBDA [](auto &&) { FAIL() << "cannot create client"; }

class YamuxedConnectionIntegrationTest : public TransportFixture {
 public:
  void SetUp() override {
    libp2p::testing::TransportFixture::SetUp();

    // test fixture is going to create Yamux for the received connection
    this->server(
        [this](std::shared_ptr<RawConnection> conn) mutable {
          EXPECT_OUTCOME_TRUE(sec_conn,
                              security_adaptor_->secureInbound(std::move(conn)))
          EXPECT_OUTCOME_TRUE(
              mux_conn,
              muxer_adaptor_->muxConnection(
                  std::move(sec_conn), [this](std::shared_ptr<Stream> s) {
                    accepted_streams_.push_back(std::move(s));
                  }))
          yamuxed_connection_ = std::move(mux_conn);
          invokeCallbacks();
          // with high probability will return EOF error, but maybe not
          (void)yamuxed_connection_->start();
          return outcome::success();
        },
        [](auto &&) { FAIL() << "cannot create server"; });
  }

  /**
   * Add a callback, which is called, when the connection is dialed and yamuxed
   * @param cb to be added
   */
  void addYamuxCallback(std::function<void()> cb) {
    if (yamuxed_connection_) {
      return cb();
    }
    yamux_callbacks_.push_back(std::move(cb));
  }

  void invokeCallbacks() {
    std::for_each(yamux_callbacks_.begin(), yamux_callbacks_.end(),
                  [](const auto &cb) { cb(); });
    yamux_callbacks_.clear();
  }

  std::shared_ptr<Stream> newStream(
      std::shared_ptr<ReadWriteCloser> conn,
      YamuxedConnection::StreamId stream_id = kDefaulExpectedStreamId) {
    std::future<std::shared_ptr<Stream>> stream_future = std::async(
        std::launch::deferred,
        [this, conn = std::move(conn), stream_id]() mutable {

          EXPECT_OUTCOME_TRUE(stream, yamuxed_connection_->newStream())
          auto new_stream_msg = newStreamMsg(stream_id);
          EXPECT_OUTCOME_TRUE(stream_msg, conn->read(YamuxFrame::kHeaderLength))
          EXPECT_EQ(new_stream_msg, stream_msg);
          return stream;
        });
    return stream_future.get();

//    std::promise<std::shared_ptr<Stream>> stream_promise;
//    auto fut = stream_promise.get_future();
//
//    addYamuxCallback(
//        [this, conn = std::move(conn), stream_id, &stream_promise]() mutable {
//          EXPECT_OUTCOME_TRUE(stream, yamuxed_connection_->newStream())
//          auto new_stream_msg = newStreamMsg(stream_id);
//          EXPECT_OUTCOME_TRUE(stream_msg, conn->read(YamuxFrame::kHeaderLength))
//          EXPECT_EQ(new_stream_msg, stream_msg);
//
//          stream_promise.set_value(stream);
//        });
//
//    fut.wait();
//    return fut.get();
  }

  std::shared_ptr<CapableConnection> yamuxed_connection_;
  std::vector<std::shared_ptr<Stream>> accepted_streams_;

  std::shared_ptr<SecurityAdaptor> security_adaptor_ =
      std::make_shared<Plaintext>();
  std::shared_ptr<MuxerAdaptor> muxer_adaptor_ = std::make_shared<Yamux>();

  /// to be set to true, when client's code is finished
  bool client_finished = false;

  std::vector<std::function<void()>> yamux_callbacks_;

  static constexpr YamuxedConnection::StreamId kDefaulExpectedStreamId = 2;
};

/**
 * @given initialized Yamux
 * @when creating a new stream from the client's side
 * @then stream is created @and corresponding ack message is sent to the client
 */
TEST_F(YamuxedConnectionIntegrationTest, StreamFromClient) {
  constexpr YamuxedConnection::StreamId created_stream_id = 1;

  auto new_stream_msg = newStreamMsg(created_stream_id);
  this->client(
      [this, &new_stream_msg,
       &created_stream_id](std::shared_ptr<RawConnection> conn) mutable {
        // open a stream, read the ack and make sure the stream is really
        // created
        EXPECT_TRUE(conn->write(new_stream_msg));

        EXPECT_OUTCOME_TRUE(ack_msg, conn->read(YamuxFrame::kHeaderLength))
        auto parsed_ack_msg = parseFrame(ack_msg);
        EXPECT_TRUE(parsed_ack_msg);
        EXPECT_EQ(parsed_ack_msg->stream_id_, created_stream_id);

        EXPECT_EQ(accepted_streams_.size(), 1);

        client_finished = true;
        return outcome::success();
      },
      CLIENT_FAIL_LAMBDA);

  launchContext();
  ASSERT_TRUE(client_finished);
}

/**
 * @given initialized Yamux
 * @when creating a new stream from the server's side
 * @then stream is created @and corresponding new stream message is received by
 * the client
 */
TEST_F(YamuxedConnectionIntegrationTest, StreamFromServer) {
  constexpr YamuxedConnection::StreamId expected_stream_id = 2;

  auto expected_new_stream_msg = newStreamMsg(expected_stream_id);
  this->client(
      [this, &expected_new_stream_msg](auto &&conn) mutable {
        // create a new stream
        addYamuxCallback([this]() {
          EXPECT_OUTCOME_TRUE(stream, yamuxed_connection_->newStream())
          EXPECT_FALSE(stream->isClosedForRead());
          EXPECT_FALSE(stream->isClosedForWrite());
          EXPECT_FALSE(stream->isClosed());
        });

        // check the client has received a message about that stream
        EXPECT_OUTCOME_TRUE(new_stream_msg,
                            conn->read(YamuxFrame::kHeaderLength))
        EXPECT_EQ(new_stream_msg, expected_new_stream_msg.toVector());

        client_finished = true;
        return outcome::success();
      },
      CLIENT_FAIL_LAMBDA);

  launchContext();
  ASSERT_TRUE(client_finished);
}

/**
 * @given initialized Yamux @and streams, multiplexed by that Yamux
 * @When writing to that stream
 * @then the operation is succesfully executed
 */
TEST_F(YamuxedConnectionIntegrationTest, StreamWrite) {
  Buffer data{{0x12, 0x34, 0xAA}};
  auto expected_data_msg = dataMsg(kDefaulExpectedStreamId, data);

  this->client(
      [this, &expected_data_msg, &data](auto &&conn) {
        auto stream = newStream(conn);

        EXPECT_TRUE(stream->write(data));
        EXPECT_OUTCOME_TRUE(received_data, conn->read(expected_data_msg.size()))
        EXPECT_EQ(received_data, expected_data_msg.toVector());

        return outcome::success();
      },
      CLIENT_FAIL_LAMBDA);

  launchContext();
}

///**
// * @given initialized Yamux @and streams, multiplexed by that Yamux
// * @when reading from that stream
// * @then the operation is successfully executed
// */
// TEST_F(YamuxIntegrationTest, StreamRead) {
//  auto stream = getNewStream();
//
//  Buffer data{{0x12, 0x34, 0xAA}};
//  auto written_data_msg = dataMsg(kDefaulExpectedStreamId, data);
//
//  connection_->asyncWrite(
//      boost::asio::buffer(written_data_msg.toVector()),
//      [&written_data_msg, &stream, &data](auto &&ec, auto &&n) mutable {
//        CHECK_IO_SUCCESS(ec, n, written_data_msg.size())
//
//        stream->readAsync([&data](Stream::NetworkMessageOutcome msg_res) {
//          ASSERT_TRUE(msg_res);
//          ASSERT_EQ(msg_res.value(), data);
//        });
//      });
//
//  launchContext();
//}
//
///**
// * @given initialized Yamux @and stream over it
// * @when closing that stream for writes
// * @then the stream is closed for writes @and corresponding message is
// received
// * on the other side
// */
// TEST_F(YamuxIntegrationTest, CloseForWrites) {
//  auto stream = getNewStream();
//
//  ASSERT_FALSE(stream->isClosedForWrite());
//  stream->close();
//  ASSERT_TRUE(stream->isClosedForWrite());
//
//  auto expected_close_stream_msg = closeStreamMsg(kDefaulExpectedStreamId);
//  auto close_stream_msg_rcv =
//      std::make_shared<Buffer>(YamuxFrame::kHeaderLength, 0);
//
//  connection_->asyncRead(
//      boost::asio::buffer(close_stream_msg_rcv->toVector()),
//      YamuxFrame::kHeaderLength,
//      [&expected_close_stream_msg, close_stream_msg_rcv](auto &&ec, auto &&n)
//      {
//        CHECK_IO_SUCCESS(ec, n, YamuxFrame::kHeaderLength)
//
//        ASSERT_EQ(*close_stream_msg_rcv, expected_close_stream_msg);
//      });
//
//  launchContext();
//}
//
///**
// * @given initialized Yamux @and stream over it
// * @when the other side sends a close message for that stream
// * @then the stream is closed for reads
// */
// TEST_F(YamuxIntegrationTest, CloseForReads) {
//  auto stream = getNewStream();
//
//  ASSERT_FALSE(stream->isClosedForRead());
//
//  auto sent_close_stream_msg = closeStreamMsg(kDefaulExpectedStreamId);
//
//  connection_->asyncWrite(boost::asio::buffer(sent_close_stream_msg.toVector()),
//                          [&sent_close_stream_msg](auto &&ec, auto &&n) {
//                            CHECK_IO_SUCCESS(ec, n,
//                                             sent_close_stream_msg.size())
//                          });
//
//  launchContext();
//  ASSERT_TRUE(stream->isClosedForRead());
//}
//
///**
// * @given initialized Yamux @and stream over it
// * @when close message is sent over the stream @and the other side responses
// * with a close message as well
// * @then the stream is closed entirely - removed from Yamux
// */
// TEST_F(YamuxIntegrationTest, CloseEntirely) {
//  auto stream = getNewStream();
//
//  ASSERT_FALSE(stream->isClosedForWrite());
//  stream->close();
//  ASSERT_TRUE(stream->isClosedForWrite());
//
//  auto expected_close_stream_msg = closeStreamMsg(kDefaulExpectedStreamId);
//  auto close_stream_msg_rcv =
//      std::make_shared<Buffer>(YamuxFrame::kHeaderLength, 0);
//
//  connection_->asyncRead(
//      boost::asio::buffer(close_stream_msg_rcv->toVector()),
//      YamuxFrame::kHeaderLength,
//      [this, &expected_close_stream_msg, close_stream_msg_rcv](auto &&ec,
//                                                               auto &&n) {
//        CHECK_IO_SUCCESS(ec, n, YamuxFrame::kHeaderLength)
//        ASSERT_EQ(*close_stream_msg_rcv, expected_close_stream_msg);
//
//        connection_->asyncWrite(
//            boost::asio::buffer(expected_close_stream_msg.toVector()),
//            [&expected_close_stream_msg](auto &&ec, auto &&n) {
//              CHECK_IO_SUCCESS(ec, n, expected_close_stream_msg.size())
//            });
//      });
//
//  launchContext();
//  ASSERT_TRUE(stream->isClosedEntirely());
//}
//
///**
// * @given initialized Yamux
// * @when a ping message arrives to Yamux
// * @then Yamux sends a ping response back
// */
// TEST_F(YamuxIntegrationTest, Ping) {
//  static constexpr uint32_t ping_value = 42;
//
//  auto ping_in_msg = pingOutMsg(ping_value);
//  auto ping_out_msg = pingResponseMsg(ping_value);
//  auto received_ping = std::make_shared<Buffer>(ping_out_msg.size(), 0);
//
//  connection_->asyncWrite(boost::asio::buffer(ping_in_msg.toVector()),
//                          [&ping_in_msg](auto &&ec, auto &&n) {
//                            CHECK_IO_SUCCESS(ec, n, ping_in_msg.size())
//                          });
//  connection_->asyncRead(boost::asio::buffer(received_ping->toVector()),
//                         ping_out_msg.size(),
//                         [&ping_out_msg, received_ping](auto &&ec, auto &&n) {
//                           CHECK_IO_SUCCESS(ec, n, ping_out_msg.size())
//                           ASSERT_EQ(*received_ping, ping_out_msg);
//                         });
//  launchContext();
//}
//
///**
// * @given initialized Yamux @and stream over it
// * @when a reset message is sent over that stream
// * @then the stream is closed entirely - removed from Yamux @and the other
// side
// * receives a corresponding message
// */
// TEST_F(YamuxIntegrationTest, Reset) {
//  auto stream = getNewStream();
//
//  ASSERT_FALSE(stream->isClosedEntirely());
//  stream->reset();
//  ASSERT_TRUE(stream->isClosedEntirely());
//
//  auto expected_reset_msg = resetStreamMsg(kDefaulExpectedStreamId);
//  auto rcvd_msg = std::make_shared<Buffer>(expected_reset_msg.size(), 0);
//
//  connection_->asyncRead(boost::asio::buffer(rcvd_msg->toVector()),
//                         expected_reset_msg.size(),
//                         [&expected_reset_msg, rcvd_msg](auto &&ec, auto &&n)
//                         {
//                           CHECK_IO_SUCCESS(ec, n, rcvd_msg->size())
//                           ASSERT_EQ(*rcvd_msg, expected_reset_msg);
//                         });
//
//  launchContext();
//}
//
///**
// * @given initialized Yamux
// * @when Yamux is closed
// * @then an underlying connection is closed @and the other side receives a
// * corresponding message
// */
// TEST_F(YamuxIntegrationTest, GoAway) {
//  yamux_->close();
//  ASSERT_TRUE(yamux_->isClosed());
//}
