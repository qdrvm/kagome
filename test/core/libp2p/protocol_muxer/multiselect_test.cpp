/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/protocol_muxer/multiselect.hpp"

#include <gtest/gtest.h>
#include <testutil/outcome.hpp>
#include "core/libp2p/transport_fixture/transport_fixture.hpp"
#include "libp2p/connection/capable_connection.hpp"
#include "mock/libp2p/connection/stream_mock.hpp"

using kagome::common::Buffer;
using libp2p::basic::ReadWriteCloser;
using libp2p::connection::CapableConnection;
using libp2p::connection::RawConnection;
using libp2p::connection::SecureConnection;
using libp2p::peer::Protocol;
using libp2p::protocol_muxer::MessageManager;
using libp2p::protocol_muxer::Multiselect;
using libp2p::protocol_muxer::ProtocolMuxer;

class MultiselectTest : public libp2p::testing::TransportFixture {
 public:
  void SetUp() override {
    libp2p::testing::TransportFixture::SetUp();

    // connection for "another" peer
    transport_listener_ = transport_->createListener(
        [this](auto conn) {
          this->other_peers_connection_ = std::move(conn);
          return outcome::success();
        },
        [](auto &&) { FAIL() << "cannot create listener"; });
    defaultDial();
    launchContext();

    ASSERT_TRUE(connection_);
    ASSERT_TRUE(other_peers_connection_);
  }

  const Protocol kDefaultEncryptionProtocol1 = "/plaintext/1.0.0";
  const Protocol kDefaultEncryptionProtocol2 = "/plaintext/2.0.0";
  const Protocol kDefaultMultiplexerProtocol = "/mplex/6.7.0";
  const Protocol kDefaultStreamProtocol = "/http/2.2.8";

  std::shared_ptr<Multiselect> multiselect_ = std::make_shared<Multiselect>();
  std::shared_ptr<ReadWriteCloser> other_peers_connection_;

  /**
   * Exchange opening messages with the other side
   */
  void negotiationOpenings() const {
    auto expected_opening_msg = MessageManager::openingMsg();

    EXPECT_OUTCOME_TRUE(
        read_msg, other_peers_connection_->read(expected_opening_msg.size()))
    ASSERT_EQ(read_msg, expected_opening_msg.toVector());

    EXPECT_OUTCOME_TRUE(
        written_bytes,
        other_peers_connection_->write(expected_opening_msg.toVector()))
    ASSERT_EQ(written_bytes, expected_opening_msg.size());
  }

  /**
   * @see negotiationOpenings()
   * @param stream to be negotiated over
   */
  //  void negotiationOpenings(const Stream &stream) {
  //    auto expected_opening_msg =
  //        std::make_shared<Buffer>(MessageManager::openingMsg());
  //    stream.readAsync(
  //        [&stream, expected_opening_msg](Stream::NetworkMessageOutcome
  //        msg_res) {
  //          EXPECT_OUTCOME_TRUE(msg, msg_res)
  //          ASSERT_EQ(msg, *expected_opening_msg);
  //
  //          stream.writeAsync(
  //              *expected_opening_msg,
  //              [expected_opening_msg](const std::error_code &ec, size_t n) {
  //                CHECK_IO_SUCCESS(ec, n, expected_opening_msg->size())
  //              });
  //        });
  //  }

  /**
   * Expect our side to send ls and respond with a list of protocols
   * @param protos_to_send - protocols, which are going to be sent
   */
  void negotiationLs(gsl::span<const Protocol> protos_to_send) {
    auto expected_ls_msg = MessageManager::lsMsg();
    auto protocols_msg = MessageManager::protocolsMsg(protos_to_send);

    EXPECT_OUTCOME_TRUE(read_msg,
                        other_peers_connection_->read(expected_ls_msg.size()))
    EXPECT_EQ(read_msg, expected_ls_msg.toVector());

    EXPECT_OUTCOME_TRUE(
        written_bytes, other_peers_connection_->write(protocols_msg.toVector()))
    ASSERT_EQ(written_bytes, protocols_msg.size());
  }

  /**
   * @see negotiationLs(..)
   * @param stream to be negotiated over
   */
  //  void negotiationLs(const Stream &stream,
  //                     gsl::span<const Protocol> protos_to_send) {
  //    auto expected_ls_msg =
  //    std::make_shared<Buffer>(MessageManager::lsMsg()); auto protocols_msg =
  //        std::make_shared<Buffer>(MessageManager::protocolsMsg(protos_to_send));
  //    stream.readAsync([&stream, expected_ls_msg,
  //                      protocols_msg](Stream::NetworkMessageOutcome msg_res)
  //                      {
  //      EXPECT_OUTCOME_TRUE(msg, msg_res)
  //      ASSERT_EQ(msg, *expected_ls_msg);
  //
  //      stream.writeAsync(*protocols_msg,
  //                        [protocols_msg](const std::error_code &ec, size_t n)
  //                        {
  //                          CHECK_IO_SUCCESS(ec, n, protocols_msg->size())
  //                        });
  //    });
  //  }

  /**
   * Expect our side to send na
   */
  void negotiationNa() {
    auto expected_na_msg = MessageManager::naMsg();

    EXPECT_OUTCOME_TRUE(read_msg,
                        other_peers_connection_->read(expected_na_msg.size()))
    EXPECT_EQ(read_msg, expected_na_msg.toVector());
  }

  /**
   * Expect our side to send a protocol and respond with the same message as an
   * acknowledgement
   * @param expected_protocol - protocol, which is expected to be received and
   * sent back
   */
  void negotiationProtocols(const Protocol &expected_protocol) {
    auto expected_proto_msg = MessageManager::protocolMsg(expected_protocol);

    EXPECT_OUTCOME_TRUE(
        read_msg, other_peers_connection_->read(expected_proto_msg.size()))
    EXPECT_EQ(read_msg, expected_proto_msg.toVector());

    EXPECT_OUTCOME_TRUE(
        written_bytes,
        other_peers_connection_->write(expected_proto_msg.toVector()))
    ASSERT_EQ(written_bytes, expected_proto_msg.size());
  }

  /**
   * @see negotiationProtocols(..)
   * @param stream to be negotiated over
   */
  //  void negotiationProtocols(const Stream &stream,
  //                            const Protocol &expected_protocol) {
  //    auto expected_proto_msg = std::make_shared<Buffer>(
  //        MessageManager::protocolMsg(expected_protocol));
  //    stream.readAsync(
  //        [&stream, expected_proto_msg](Stream::NetworkMessageOutcome msg_res)
  //        {
  //          EXPECT_OUTCOME_TRUE(msg, msg_res)
  //          ASSERT_EQ(msg, *expected_proto_msg);
  //
  //          stream.writeAsync(
  //              *expected_proto_msg,
  //              [expected_proto_msg](const std::error_code &ec, size_t n) {
  //                CHECK_IO_SUCCESS(ec, n, expected_proto_msg->size())
  //              });
  //        });
  //  }
};

/**
 * @given connection, over which we want to negotiate @and multiselect instance
 * over that connection @and encryption protocol, supported by both sides
 * @when negotiating about the protocol
 * @then the common protocol is selected
 */
TEST_F(MultiselectTest, NegotiateEncryption) {
  multiselect_->addEncryptionProtocol(kDefaultEncryptionProtocol2);

  // create a success handler to be called, when a negotiation is finished
  multiselect_->negotiateEncryption(
      std::static_pointer_cast<RawConnection>(
          connection_),  // multiselect does not use anything except read/write,
                         // so it's safe
      [this](outcome::result<Protocol> protocol_res) {
        EXPECT_OUTCOME_TRUE(protocol, protocol_res)
        EXPECT_EQ(protocol, kDefaultEncryptionProtocol2);
      });

  // first, we expect an exchange of opening messages
  negotiationOpenings();

  // second, ls message will be sent to us; respond with a list of encryption
  // protocols we know
  negotiationLs(std::vector<Protocol>{kDefaultEncryptionProtocol1,
                                      kDefaultEncryptionProtocol2});

  // finally, we expect that the second of the protocols will be sent back to
  // us, as it is the common one; after that, we should send an ack
  negotiationProtocols(kDefaultEncryptionProtocol2);

  launchContext();
}

/**
 * @given connection, over which we want to negotiate @and multiselect instance
 * over that connection @and mutiplexer protocol, supported by both sides
 * @when negotiating about the protocol
 * @then the common protocol is selected
 */
TEST_F(MultiselectTest, NegotiateMultiplexer) {
  multiselect_->addMultiplexerProtocol(kDefaultMultiplexerProtocol);

  // create a success handler to be called, when a negotiation is finished
  multiselect_->negotiateMultiplexer(
      std::static_pointer_cast<SecureConnection>(connection_),
      [this](outcome::result<Protocol> protocol_res) {
        EXPECT_OUTCOME_TRUE(protocol, protocol_res)
        EXPECT_EQ(protocol, kDefaultMultiplexerProtocol);
      });

  negotiationOpenings();
  negotiationLs(std::vector<Protocol>{kDefaultMultiplexerProtocol});
  negotiationProtocols(kDefaultMultiplexerProtocol);

  launchContext();
}

/**
 * @given stream, over which we want to negotiate @and multiselect instance
 * over that connection @and stream protocol, supported by both sides
 * @when negotiating about the protocol
 * @then the common protocol is selected
 */
// TEST_F(MultiselectTest, NegotiateStream) {
//  // create a lambda, which is going to accept another end's stream, created
//  by
//  // Yamux, and participate in negotiations via that stream
//  std::unique_ptr<Stream> received_stream;
//  auto stream_handler =
//      [this, &received_stream](std::unique_ptr<Stream> stream) mutable {
//        negotiationOpenings(*stream);
//        negotiationLs(*stream, std::vector<Protocol>{kDefaultStreamProtocol});
//        negotiationProtocols(*stream, kDefaultStreamProtocol);
//
//        // prolongate life of the stream
//        received_stream = std::move(stream);
//      };
//
//  // set up Yamuxes
//  std::shared_ptr<Yamux> yamux1{std::make_shared<Yamux>(
//      connection_, [](auto &&) {}, YamuxConfig{false})},
//      yamux2{std::make_shared<Yamux>(other_peers_connection_,
//                                     std::move(stream_handler),
//                                     YamuxConfig{true})};
//  yamux1->start();
//  yamux2->start();
//  EXPECT_OUTCOME_TRUE(stream1, yamux1->newStream())
//
//  // create a success handler
//  multiselect_->addStreamProtocol(kDefaultStreamProtocol);
//  multiselect_->negotiateStream(
//      std::move(stream1),
//      [this, &stream1](outcome::result<Protocol> protocol_res,  // NOLINT
//                       std::unique_ptr<Stream> stream) mutable {
//        EXPECT_OUTCOME_TRUE(protocol, protocol_res)
//        EXPECT_EQ(protocol, kDefaultStreamProtocol);
//        stream1 = std::move(stream);
//      });
//
//  launchContext();
//}

/**
 * @given connection, over which we want to negotiate @and multiselect instance
 * over that connection @and encryption protocol, not supported by our side
 * @when negotiating about the protocol
 * @then the common protocol is not selected
 */
TEST_F(MultiselectTest, NegotiateFailure) {
  multiselect_->addEncryptionProtocol(kDefaultEncryptionProtocol1);

  // create a failure handler to be called, when a negotiation is finished
  multiselect_->negotiateEncryption(
      std::static_pointer_cast<RawConnection>(connection_),
      [](outcome::result<Protocol> protocol_res) {
        EXPECT_FALSE(protocol_res);
      });

  negotiationOpenings();
  // send a protocol, which is not supported by us
  negotiationLs(std::vector<Protocol>{kDefaultEncryptionProtocol2});
  // expect na message to be sent by us
  negotiationNa();

  launchContext();
}

/**
 * @given connection, over which we want to negotiate @and multiselect instance
 * over that connection @and no protocols, supported by our side
 * @when negotiating about the protocol
 * @then the common protocol is not selected
 */
TEST_F(MultiselectTest, NoProtocols) {
  // create a failure handler, which is going to be called immediately
  multiselect_->negotiateEncryption(
      std::static_pointer_cast<RawConnection>(connection_),
      [](outcome::result<Protocol> protocol_res) {
        EXPECT_FALSE(protocol_res);
      });
}
