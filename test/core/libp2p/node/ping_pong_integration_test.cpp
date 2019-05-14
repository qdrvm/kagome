/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <memory>

#include <gtest/gtest.h>
#include <boost/asio/io_context.hpp>
#include <testutil/outcome.hpp>
#include "libp2p/muxer/yamux.hpp"
#include "libp2p/transport/all.hpp"
#include "libp2p/host.hpp"
#include "libp2p/host_builder.hpp"
#include "libp2p/peer/protocol.hpp"


using namespace libp2p;
using namespace libp2p::transport;
using namespace libp2p::muxer;
using namespace libp2p::stream;
using namespace kagome::common;
using namespace libp2p::multi;
using namespace libp2p::peer;

/**
 * Test, emulating two nodes exchanging ping-pong messages
 */
class PingPongIntegrationTest : public ::testing::Test {
 public:
  boost::asio::io_context context_;

  const Protocol kDefaultProtocol = "/default-proto/1.0.0";

  const Buffer kPingMsg = Buffer{}.put("PING");
  const Buffer kPongMsg = Buffer{}.put("PONG");

  const Multiaddress kFirstMultiaddress =
      Multiaddress::create("/ip4/0.0.0.0/tcp/0").value();
  const Multiaddress kSecondMultiaddress =
      Multiaddress::create("/ip4/0.0.0.0/tcp/0").value();
};

TEST_F(PingPongIntegrationTest, PingPong) {
  auto node1 = std::make_shared<Node>();
  auto node2 = std::make_shared<Node>();

  // both nodes are to support TCP
  node1->addTransport(std::make_unique<TransportImpl>(context_));
  node2->addTransport(std::make_unique<TransportImpl>(context_));

  // both nodes are to support YamuxedConnection protocol
  // for now, we assume first node is a server, and the second is a client
  node1->addMuxer(std::make_unique<Yamux>(YamuxConfig{true}));
  node2->addMuxer(std::make_unique<Yamux>(YamuxConfig{false}));

  // make the nodes listen on their addresses
  node1->listen(kFirstMultiaddress);
  node2->listen(kSecondMultiaddress);

  // streams holders, so that during the async events they do not get killed
  std::unique_ptr<Stream> stream1, stream2;

  // the first node is a listener, responding to PING message
  node1->handle(
      kDefaultProtocol,
      [this, &stream1](std::unique_ptr<Stream> stream, PeerId peer_id) mutable {
        // we accepted a connection, so wait for the message
        auto &stream_ref = *stream;
        stream_ref.readAsync(
            [this, stream = std::move(stream),
             &stream1](Stream::NetworkMessageOutcome msg_res) mutable {
              EXPECT_OUTCOME_TRUE(msg, msg_res)
              EXPECT_EQ(msg, kPingMsg);
              stream->writeAsync(kPongMsg);
              stream1 = std::move(stream);
            });
      });

  // second node is an initiator of the connection, sending the PING message
  node2->dial(
      node1->peerInfo(), kDefaultProtocol,
      [this,
       &stream2](outcome::result<std::unique_ptr<Stream>> stream_res) mutable {
        EXPECT_OUTCOME_TRUE(stream, stream_res)
        stream->writeAsync(kPingMsg);
        stream->readAsync([this](Stream::NetworkMessageOutcome msg_res) {
          EXPECT_OUTCOME_TRUE(msg, msg_res)
          EXPECT_EQ(msg, kPongMsg);
        });
        stream2 = std::move(stream);
      });

  using std::chrono_literals::operator""ms;
  context_.run_for(50ms);
}
