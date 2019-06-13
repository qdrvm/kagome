/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/connection/yamux/yamuxed_connection.hpp"

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

#define STREAM_WRITE_CB [](auto &&res) { ASSERT_TRUE(res); }
#define READ_STREAM_OPENING read(YamuxFrame::kHeaderLength, [](auto &&) {})

class YamuxedConnectionIntegrationTest : public TransportFixture {
 public:
  void SetUp() override {
    libp2p::testing::TransportFixture::SetUp();

    // test fixture is going to create Yamux for the received connection
    this->server([this](auto &&conn_res) mutable {
      EXPECT_OUTCOME_TRUE(conn, conn_res)
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
      (void)yamuxed_connection_->start();
      return outcome::success();
    });
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

  /**
   * Invoke all callbacks, which were waiting for the connection to be yamuxed
   */
  void invokeCallbacks() {
    std::for_each(yamux_callbacks_.begin(), yamux_callbacks_.end(),
                  [](const auto &cb) { cb(); });
    yamux_callbacks_.clear();
  }

  /**
   * Open a new stream; assumes yamuxed_connection_ exists
   */
  void newStream(
      std::shared_ptr<ReadWriteCloser> conn,
      std::function<CapableConnection::StreamResultHandler> cb,
      YamuxedConnection::StreamId stream_id = kDefaulExpectedStreamId) {
    yamuxed_connection_->newStream(
        [conn, stream_id, cb = std::move(cb)](auto &&stream_res) {
          EXPECT_OUTCOME_TRUE(stream, stream_res)
          EXPECT_OUTCOME_TRUE(stream_msg, conn->read(YamuxFrame::kHeaderLength))

          auto new_stream_msg = newStreamMsg(stream_id);
          EXPECT_EQ(new_stream_msg, stream_msg);

          cb(std::move(stream));
        });
  }

  std::shared_ptr<CapableConnection> yamuxed_connection_;
  std::vector<std::shared_ptr<Stream>> accepted_streams_;

  std::shared_ptr<SecurityAdaptor> security_adaptor_ =
      std::make_shared<Plaintext>();
  std::shared_ptr<MuxerAdaptor> muxer_adaptor_ = std::make_shared<Yamux>();

  /// to be set to true, when client's code is finished
  bool client_finished = false;

  std::vector<std::function<void()>> yamux_callbacks_;

  const std::vector<uint8_t> kSyncToken{0x11};

  static constexpr YamuxedConnection::StreamId kDefaulExpectedStreamId = 2;

  std::shared_ptr<ReadWriteCloser> other_connection_;
  std::queue<std::pair<size_t, std::function<void(std::vector<uint8_t>)>>>
      pending_reads_;
  bool is_reading_ = false;

  void read(size_t bytes, std::function<void(std::vector<uint8_t>)> cb) {
    ASSERT_TRUE(other_connection_);
    pending_reads_.push({bytes, std::move(cb)});
    if (is_reading_) {
      return;
    }
    is_reading_ = true;
    while (!pending_reads_.empty()) {
      auto read_req = pending_reads_.front();
      EXPECT_OUTCOME_TRUE(data, other_connection_->read(read_req.first));
      read_req.second(std::move(data));
      pending_reads_.pop();
    }
    is_reading_ = false;
  }
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
      [this, &new_stream_msg, &created_stream_id](auto &&conn_res) mutable {
        EXPECT_OUTCOME_TRUE(conn, conn_res)

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
      });

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
  this->client([this, &expected_new_stream_msg](auto &&conn_res) mutable {
    EXPECT_OUTCOME_TRUE(conn, conn_res)
    // create a new stream
    addYamuxCallback([this]() {
      yamuxed_connection_->newStream([](auto &&stream_res) {
        EXPECT_OUTCOME_TRUE(stream, stream_res)
        EXPECT_FALSE(stream->isClosedForRead());
        EXPECT_FALSE(stream->isClosedForWrite());
        EXPECT_FALSE(stream->isClosed());
      });
    });

    // check the client has received a message about that stream
    EXPECT_OUTCOME_TRUE(new_stream_msg, conn->read(YamuxFrame::kHeaderLength))
    EXPECT_EQ(new_stream_msg, expected_new_stream_msg.toVector());

    client_finished = true;
    return outcome::success();
  });

  launchContext();
  ASSERT_TRUE(client_finished);
}

/**
 * @given initialized Yamux @and streams, multiplexed by that Yamux
 * @When writing to that stream
 * @then the operation is successfully executed
 */
TEST_F(YamuxedConnectionIntegrationTest, StreamWrite) {
  Buffer data{{0x12, 0x34, 0xAA}};
  auto expected_data_msg = dataMsg(kDefaulExpectedStreamId, data);

  this->client([this, &expected_data_msg, &data](auto &&conn_res) {
    EXPECT_OUTCOME_TRUE(conn, conn_res)
    other_connection_ = conn;
    addYamuxCallback([this, &expected_data_msg, &data, conn]() mutable {
      yamuxed_connection_->newStream(
          [this, &expected_data_msg, &data, conn](auto &&stream_res) {
            EXPECT_OUTCOME_TRUE(stream, stream_res)

            stream->write(data).then(STREAM_WRITE_CB);
            read(expected_data_msg.size(),
                 [this, &expected_data_msg](auto received_data) {
                   EXPECT_EQ(received_data, expected_data_msg.toVector());
                   client_finished = true;
                 });
          });
    });
    READ_STREAM_OPENING;
    return outcome::success();
  });

  launchContext();
  ASSERT_TRUE(client_finished);
}

/**
 * @given initialized Yamux @and streams, multiplexed by that Yamux
 * @when reading from that stream
 * @then the operation is successfully executed
 */
TEST_F(YamuxedConnectionIntegrationTest, StreamRead) {
  Buffer data{{0x12, 0x34, 0xAA}};
  auto written_data_msg = dataMsg(kDefaulExpectedStreamId, data);

  this->client([this, &written_data_msg, &data](auto &&conn_res) {
    EXPECT_OUTCOME_TRUE(conn, conn_res)
    other_connection_ = conn;
    addYamuxCallback([this, &data, conn] {
      yamuxed_connection_->newStream([this, &data](auto &&stream_res) {
        EXPECT_OUTCOME_TRUE(stream, stream_res)
        stream->read(data.size()).then([this, &data, stream](auto &&read_data) {
          ASSERT_TRUE(read_data);
          ASSERT_EQ(data.toVector(), read_data.value());
          client_finished = true;
        });
      });
    });
    READ_STREAM_OPENING;
    EXPECT_TRUE(conn->write(written_data_msg));
    return outcome::success();
  });

  launchContext();
  ASSERT_TRUE(client_finished);
}

/**
 * @given initialized Yamux @and stream over it
 * @when closing that stream for writes
 * @then the stream is closed for writes @and corresponding message is
 received
 * on the other side
 */
TEST_F(YamuxedConnectionIntegrationTest, CloseForWrites) {
  auto expected_close_stream_msg = closeStreamMsg(kDefaulExpectedStreamId);

  this->client([this, &expected_close_stream_msg](auto &&conn_res) {
    EXPECT_OUTCOME_TRUE(conn, conn_res)
    other_connection_ = conn;
    addYamuxCallback([this, &expected_close_stream_msg, conn] {
      yamuxed_connection_->newStream([this, &expected_close_stream_msg](
                                         auto &&stream_res) {
        EXPECT_OUTCOME_TRUE(stream, stream_res)
        ASSERT_FALSE(stream->isClosedForWrite());
        stream->close().then([stream](auto &&res) {
          ASSERT_TRUE(res);
          ASSERT_TRUE(stream->isClosedForWrite());
        });

        read(expected_close_stream_msg.size(),
             [this, &expected_close_stream_msg](auto received_data) {
               ASSERT_EQ(received_data, expected_close_stream_msg.toVector());
               client_finished = true;
             });
      });
    });
    READ_STREAM_OPENING;
    return outcome::success();
  });

  launchContext();
  ASSERT_TRUE(client_finished);
}

/**
 * @given initialized Yamux @and stream over it
 * @when the other side sends a close message for that stream
 * @then the stream is closed for reads
 */
TEST_F(YamuxedConnectionIntegrationTest, CloseForReads) {
  auto sent_close_stream_msg = closeStreamMsg(kDefaulExpectedStreamId);

  std::shared_ptr<Stream> stream_out;
  this->client(
      [this, &sent_close_stream_msg, &stream_out](auto &&conn_res) mutable {
        EXPECT_OUTCOME_TRUE(conn, conn_res)
        other_connection_ = conn;
        addYamuxCallback([this, &stream_out]() mutable {
          yamuxed_connection_->newStream(
              [this, &stream_out](auto &&stream_res) mutable {
                EXPECT_OUTCOME_TRUE(stream, stream_res)
                ASSERT_FALSE(stream->isClosedForRead());
                stream->write(kSyncToken)
                    .then([this, stream, &stream_out](auto &&res) mutable {
                      ASSERT_TRUE(res);
                      client_finished = true;
                      stream_out = std::move(stream);
                    });
              });
        });
        READ_STREAM_OPENING;
        EXPECT_TRUE(conn->read(YamuxFrame::kHeaderLength + kSyncToken.size()));
        EXPECT_TRUE(conn->write(sent_close_stream_msg));
        return outcome::success();
      });

  launchContext();
  ASSERT_TRUE(client_finished);
  ASSERT_TRUE(stream_out);
  ASSERT_TRUE(stream_out->isClosedForRead());
}

/**
 * @given initialized Yamux @and stream over it
 * @when reset message is sent over the connection
 * @then the stream is closed entirely - removed from Yamux
 */
TEST_F(YamuxedConnectionIntegrationTest, Reset) {
  auto reset_stream_msg = resetStreamMsg(kDefaulExpectedStreamId);

  std::shared_ptr<Stream> stream_out;
  this->client([this, &reset_stream_msg, &stream_out](auto &&conn_res) mutable {
    EXPECT_OUTCOME_TRUE(conn, conn_res)
    other_connection_ = conn;
    addYamuxCallback([this, &stream_out]() mutable {
      yamuxed_connection_->newStream(
          [this, &stream_out](auto &&stream_res) mutable {
            EXPECT_OUTCOME_TRUE(stream, stream_res)
            ASSERT_FALSE(stream->isClosed());
            stream->write(kSyncToken)
                .then([this, stream, &stream_out](auto &&res) mutable {
                  ASSERT_TRUE(res);
                  client_finished = true;
                  stream_out = std::move(stream);
                });
          });
    });
    READ_STREAM_OPENING;
    EXPECT_TRUE(conn->read(YamuxFrame::kHeaderLength + kSyncToken.size()));
    EXPECT_TRUE(conn->write(reset_stream_msg));
    return outcome::success();
  });

  launchContext();
  ASSERT_TRUE(client_finished);
  ASSERT_TRUE(stream_out->isClosed());
}

/**
 * @given initialized Yamux
 * @when a ping message arrives to Yamux
 * @then Yamux sends a ping response back
 */
TEST_F(YamuxedConnectionIntegrationTest, Ping) {
  static constexpr uint32_t ping_value = 42;

  auto ping_in_msg = pingOutMsg(ping_value);
  auto ping_out_msg = pingResponseMsg(ping_value);

  this->client([this, &ping_in_msg, &ping_out_msg](auto &&conn_res) {
    EXPECT_OUTCOME_TRUE(conn, conn_res)
    EXPECT_TRUE(conn->write(ping_in_msg));
    EXPECT_OUTCOME_TRUE(received_msg, conn->read(ping_out_msg.size()))
    EXPECT_EQ(received_msg, ping_out_msg.toVector());
    client_finished = true;
    return outcome::success();
  });

  launchContext();
  ASSERT_TRUE(client_finished);
}
