/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <memory>

#include <gtest/gtest.h>
#include <boost/asio/io_context.hpp>
#include <testutil/outcome.hpp>
#include "libp2p/host.hpp"
#include "libp2p/host_builder.hpp"
#include "libp2p/muxer/yamux.hpp"
#include "libp2p/peer/protocol.hpp"
#include "libp2p/transport/all.hpp"

using namespace libp2p;
using namespace libp2p::transport;
using namespace libp2p::muxer;
using namespace libp2p::stream;
using namespace kagome::common;
using namespace libp2p::multi;

using std::chrono_literals::operator""ms;

inline Buffer operator"" _buf(const char *c, size_t s) {
  return Buffer{}.put({c, c + s});
}

inline multi::Multiaddress operator"" _addr(const char *c, size_t s) {
  auto r = multi::Multiaddress::create(std::string{c, c + s});
  assert(r.has_value());
  return r.value();
}

/**
 * Test, emulating two nodes exchanging ping-pong messages
 */
class PingPongIntegrationTest : public ::testing::Test {
 public:
  boost::asio::io_context context_;

  const peer::Protocol kDefaultProtocol = "/default-proto/1.0.0";

  const Buffer kPingMsg = "PING"_buf;
  const Buffer kPongMsg = "PONG"_buf;

  const Multiaddress ma1 = "/ip4/0.0.0.0/tcp/0"_addr;
  const Multiaddress ma2 = "/ip4/0.0.0.0/tcp/0"_addr;

  Host make_host(const multi::Multiaddress &ma) {
    // TODO: change transport impl to be "adapter"
    auto tcp = std::make_unique<transport::TransportImpl>(context_);
    auto yamux = std::make_unique<Yamux>();

    return HostBuilder{}
        .addTransport(std::move(tcp))
        .addStreamMuxer(std::move(yamux))
        .addListenAddress(ma)
        .build();
  }
};

TEST_F(PingPongIntegrationTest, PingPong) {
  auto node1 = make_host(ma1);
  auto node2 = make_host(ma2);

  // the first node is a listener, responding to PING message
  node1.setProtocolHandler(
      kDefaultProtocol, [this](std::shared_ptr<Stream> stream) {
        // we accepted a connection, so wait for the message
        stream->readAsync(
            [this, stream](Stream::NetworkMessageOutcome msg_res) {
              EXPECT_OUTCOME_TRUE(msg, msg_res)
              EXPECT_EQ(msg, kPingMsg);
              stream->writeAsync(kPongMsg);
            });
      });

  // TODO: promisify
  // second node is an initiator of the connection, sending the PING message
  auto r = node2.newStream(
      node1.getPeerInfo(), kDefaultProtocol,
      [this](outcome::result<std::shared_ptr<Stream>> stream_res) {
        EXPECT_OUTCOME_TRUE(stream, stream_res)
        stream->writeAsync(kPingMsg);
        stream->readAsync([this](Stream::NetworkMessageOutcome msg_res) {
          EXPECT_OUTCOME_TRUE(msg, msg_res)
          EXPECT_EQ(msg, kPongMsg);
        });
      });

  ASSERT_TRUE(r);

  context_.run_for(50ms);
}
