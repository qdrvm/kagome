/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/node/libp2p_node.hpp"

#include <memory>

#include <gtest/gtest.h>
#include <boost/asio/io_context.hpp>
#include <testutil/outcome.hpp>
#include "libp2p/muxer/yamux.hpp"
#include "libp2p/transport/all.hpp"

using namespace libp2p::node;
using namespace libp2p::transport;
using namespace libp2p::muxer;
using namespace libp2p::peer;
using namespace libp2p::stream;
using namespace kagome::common;
using namespace libp2p::multi;

/**
 * Test, emulating two nodes exchanging ping-pong messages
 */
class PingPongIntegrationTest : public ::testing::Test {
 public:
  boost::asio::io_context context_;

  const libp2p::peer::Protocol kDefaultProtocol = "/default-proto/1.0.0";

  const Buffer kPingMsg = Buffer{}.put("PING");
  const Buffer kPongMsg = Buffer{}.put("PONG");

  const Multiaddress kFirstMultiaddress =
      Multiaddress::create("/ip4/0.0.0.0/tcp/0").value();
  const Multiaddress kSecondMultiaddress =
      Multiaddress::create("/ip4/0.0.0.0/tcp/0").value();
};

TEST_F(PingPongIntegrationTest, PingPong) {
  // initialize the nodes with different identities (both PeerId and
  // Multiaddresses are not necessary for connection)
  PeerInfo first_info{}, second_info{};
  first_info.addresses_.insert(kFirstMultiaddress);
  second_info.addresses_.insert(kSecondMultiaddress);

  auto first_node = std::make_shared<Libp2pNode>(std::move(first_info));
  auto second_node = std::make_shared<Libp2pNode>(std::move(second_info));

  // both nodes are to support TCP
  first_node->addTransport(std::make_unique<TransportImpl>(context_));
  second_node->addTransport(std::make_unique<TransportImpl>(context_));

  // both nodes are to support YamuxedConnection protocol
  // for now, we assume first node is a server, and the second is a client
  first_node->addMuxer(std::make_unique<Yamux>(YamuxConfig{true}));
  second_node->addMuxer(std::make_unique<Yamux>(YamuxConfig{false}));

  // make the nodes listen on their addresses
  first_node->listen(kFirstMultiaddress);
  second_node->listen(kSecondMultiaddress);

  // streams holders, so that during the async events they do not get killed
  std::unique_ptr<Stream> stream1, stream2;

  // the first node is a listener, responding to PING message
  first_node->handle(
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
  second_node->dial(
      first_node->peerInfo(), kDefaultProtocol,
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
