/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/connection/yamux/yamuxed_connection.hpp"

#include <gtest/gtest.h>
#include "common/buffer.hpp"
#include "core/libp2p/transport_fixture/transport_fixture.hpp"
#include "libp2p/muxer/yamux.hpp"
#include "libp2p/security/plaintext.hpp"
#include "testutil/outcome.hpp"

using namespace libp2p::connection;
using namespace libp2p::transport;
using namespace kagome::common;
using namespace libp2p::multi;
using namespace libp2p::basic;
using namespace libp2p::security;
using namespace libp2p::muxer;

struct ServerStream;

class YamuxAcceptanceTest : public libp2p::testing::TransportFixture {
 public:
  std::shared_ptr<SecurityAdaptor> security_adaptor_ =
      std::make_shared<Plaintext>();
  std::shared_ptr<MuxerAdaptor> muxer_adaptor_ = std::make_shared<Yamux>();

  std::shared_ptr<CapableConnection> server_connection_;
  std::shared_ptr<CapableConnection> client_connection_;

  std::vector<std::shared_ptr<ServerStream>> server_streams_;
};

const Buffer kPingBytes = Buffer{}.put("PING");
const Buffer kPongBytes = Buffer{}.put("PONG");

struct ServerStream : std::enable_shared_from_this<ServerStream> {
  explicit ServerStream(std::shared_ptr<Stream> s)
      : stream{std::move(s)}, read_buffer(kPingBytes.size(), 0) {}

  std::shared_ptr<Stream> stream;
  Buffer read_buffer;

  void doRead() {
    if (stream->isClosedForRead()) {
      return;
    }
    stream->read(read_buffer, read_buffer.size(),
                 [self = shared_from_this()](auto &&res) {
                   ASSERT_TRUE(res);
                   self->readCompleted();
                 });
  }

  void readCompleted() {
    ASSERT_EQ(read_buffer, kPingBytes) << "expected to received a PING message";
    doWrite();
  }

  void doWrite() {
    if (stream->isClosedForWrite()) {
      return;
    }
    stream->write(kPongBytes, kPongBytes.size(),
                  [self = shared_from_this()](auto &&res) {
                    ASSERT_TRUE(res);
                    self->doRead();
                  });
  }
};

/**
 * @given Yamuxed server, which is setup to write 'PONG' for any received 'PING'
 * message @and Yamuxed client, connected to that server
 * @when the client sets up a listener on that server @and writes 'PING'
 * @then the 'PONG' message is received by the client
 */
TEST_F(YamuxAcceptanceTest, PingPong) {
  // set up a Yamux server - that lambda is to be called, when a new connection
  // is received
  this->server([this](auto &&conn_res) mutable {
    // accept and upgrade the connection to the capable one
    EXPECT_OUTCOME_TRUE(conn, conn_res)
    EXPECT_OUTCOME_TRUE(sec_conn,
                        security_adaptor_->secureInbound(std::move(conn)))
    EXPECT_OUTCOME_TRUE(mux_conn,
                        muxer_adaptor_->muxConnection(
                            std::move(sec_conn),
                            [this](auto &&stream_res) {
                              // wrap each received stream into a server
                              // structure and start reading
                              ASSERT_TRUE(stream_res);
                              auto server = std::make_shared<ServerStream>(
                                  std::move(stream_res.value()));
                              server->doRead();
                              server_streams_.push_back(std::move(server));
                            },
                            MuxedConnectionConfig{}))
    server_connection_ = std::move(mux_conn);
    server_connection_->start();
    return outcome::success();
  });

  this->client([this](auto &&conn_res) {
    // accept and upgrade the connection to the capable one
    EXPECT_OUTCOME_TRUE(conn, conn_res)
    EXPECT_OUTCOME_TRUE(sec_conn,
                        security_adaptor_->secureInbound(std::move(conn)))
    EXPECT_OUTCOME_TRUE(mux_conn,
                        muxer_adaptor_->muxConnection(
                            std::move(sec_conn),
                            [](auto &&stream_res) {
                              // here we are not going to accept any streams -
                              // pure client
                              FAIL() << "no streams should be here";
                            },
                            MuxedConnectionConfig{}))
    client_connection_ = std::move(mux_conn);
    client_connection_->start();
    return outcome::success();
  });

  // let both client and server be created
  launchContext();

  auto stream_read = false, stream_wrote = false;
  Buffer stream_read_buffer(kPongBytes.size(), 0);
  client_connection_->newStream([&stream_read_buffer, &stream_read,
                                 &stream_wrote](auto &&stream_res) mutable {
    EXPECT_OUTCOME_TRUE(stream, stream_res)

    // proof our streams have parallelism: set up both read and write on the
    // stream and make sure they are successfully executed
    stream->read(stream_read_buffer, stream_read_buffer.size(),
                 [&stream_read_buffer, &stream_read](auto &&res) {
                   ASSERT_EQ(stream_read_buffer, kPongBytes);
                   stream_read = true;
                 });
    stream->write(kPingBytes, kPingBytes.size(), [&stream_wrote](auto &&res) {
      ASSERT_TRUE(res);
      stream_wrote = true;
    });
  });

  // let the streams make their jobs
  //  context_.run();
  launchContext();

  ASSERT_TRUE(stream_read);
  ASSERT_TRUE(stream_wrote);
}
