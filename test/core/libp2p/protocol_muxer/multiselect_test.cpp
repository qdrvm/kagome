/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/protocol_muxer/multiselect.hpp"

#include <gtest/gtest.h>
#include <testutil/outcome.hpp>
#include "core/libp2p/transport_fixture/transport_fixture.hpp"
#include "mock/libp2p/connection/raw_connection_mock.hpp"

using libp2p::basic::ReadWriteCloser;
using libp2p::connection::CapableConnection;
using libp2p::connection::RawConnection;
using libp2p::connection::RawConnectionMock;
using libp2p::peer::Protocol;
using libp2p::protocol_muxer::MessageManager;
using libp2p::protocol_muxer::Multiselect;
using libp2p::testing::TransportFixture;

class MultiselectTest : public TransportFixture {
 public:
  const Protocol kDefaultEncryptionProtocol1 = "/plaintext/1.0.0";
  const Protocol kDefaultEncryptionProtocol2 = "/plaintext/2.0.0";

  std::shared_ptr<Multiselect> multiselect_ = std::make_shared<Multiselect>();

  /**
   * Exchange opening messages as an initiator
   */
  static void negotiationOpeningsInitiator(ReadWriteCloser &conn) {
    auto expected_opening_msg = MessageManager::openingMsg();

    EXPECT_OUTCOME_TRUE(read_msg, conn.read(expected_opening_msg.size()))
    ASSERT_EQ(read_msg, expected_opening_msg.toVector());

    EXPECT_OUTCOME_TRUE(written_bytes, conn.write(expected_opening_msg))
    ASSERT_EQ(written_bytes, expected_opening_msg.size());
  }

  /**
   * Exchange opening messages as a listener
   * @param conn
   */
  static void negotiationOpeningsListener(ReadWriteCloser &conn) {
    auto expected_opening_msg = MessageManager::openingMsg();

    EXPECT_OUTCOME_TRUE(written_bytes, conn.write(expected_opening_msg))
    ASSERT_EQ(written_bytes, expected_opening_msg.size());

    EXPECT_OUTCOME_TRUE(read_msg, conn.read(expected_opening_msg.size()))
    ASSERT_EQ(read_msg, expected_opening_msg.toVector());
  }

  /**
   * Expect to receive an LS and respond with a list of protocols
   */
  static void negotiationLsInitiator(ReadWriteCloser &conn,
                                     gsl::span<const Protocol> protos_to_send) {
    auto expected_ls_msg = MessageManager::lsMsg();
    auto protocols_msg = MessageManager::protocolsMsg(protos_to_send);

    EXPECT_OUTCOME_TRUE(read_msg, conn.read(expected_ls_msg.size()))
    EXPECT_EQ(read_msg, expected_ls_msg.toVector());

    EXPECT_OUTCOME_TRUE(written_bytes, conn.write(protocols_msg))
    ASSERT_EQ(written_bytes, protocols_msg.size());
  }

  static void negotiationLsListener(
      ReadWriteCloser &conn, gsl::span<const Protocol> protos_to_receive) {
    auto ls_msg = MessageManager::lsMsg();
    auto protocols_msg = MessageManager::protocolsMsg(protos_to_receive);

    EXPECT_OUTCOME_TRUE(written_bytes, conn.write(ls_msg))
    ASSERT_EQ(written_bytes, ls_msg.size());

    EXPECT_OUTCOME_TRUE(read_msg, conn.read(protocols_msg.size()))
    EXPECT_EQ(read_msg, protocols_msg.toVector());
  }

  /**
   * Send a protocol and expect NA as a response
   */
  static void negotiationProtocolNaListener(ReadWriteCloser &conn,
                                            const Protocol &proto_to_send) {
    auto na_msg = MessageManager::naMsg();
    auto protocol_msg = MessageManager::protocolMsg(proto_to_send);

    EXPECT_OUTCOME_TRUE(written_bytes, conn.write(protocol_msg))
    ASSERT_EQ(written_bytes, protocol_msg.size());

    EXPECT_OUTCOME_TRUE(read_msg, conn.read(na_msg.size()))
    ASSERT_EQ(read_msg, na_msg.toVector());
  }

  /**
   * Receive a protocol msg and respond with the same message as an
   * acknowledgement
   */
  static void negotiationProtocolsInitiator(ReadWriteCloser &conn,
                                            const Protocol &expected_protocol) {
    auto expected_proto_msg = MessageManager::protocolMsg(expected_protocol);

    EXPECT_OUTCOME_TRUE(read_msg, conn.read(expected_proto_msg.size()))
    EXPECT_EQ(read_msg, expected_proto_msg.toVector());

    EXPECT_OUTCOME_TRUE(written_bytes, conn.write(expected_proto_msg))
    ASSERT_EQ(written_bytes, expected_proto_msg.size());
  }

  /**
   * Send a protocol and expect it to be received as an ack
   */
  static void negotiationProtocolsListener(ReadWriteCloser &conn,
                                           const Protocol &expected_protocol) {
    auto expected_proto_msg = MessageManager::protocolMsg(expected_protocol);

    EXPECT_OUTCOME_TRUE(written_bytes, conn.write(expected_proto_msg))
    ASSERT_EQ(written_bytes, expected_proto_msg.size());

    EXPECT_OUTCOME_TRUE(read_msg, conn.read(expected_proto_msg.size()))
    EXPECT_EQ(read_msg, expected_proto_msg.toVector());
  }
};

/**
 * @given connection, over which we want to negotiate @and multiselect instance
 * over that connection @and protocol, supported by both sides
 * @when negotiating about the protocol as an initiator side
 * @then the common protocol is selected
 */
TEST_F(MultiselectTest, NegotiateAsInitiator) {
  auto negotiated = false;

  this->server(
      [this, &negotiated](
          outcome::result<std::shared_ptr<CapableConnection>> rconn) mutable {
        EXPECT_OUTCOME_TRUE(conn, rconn);
        EXPECT_OUTCOME_TRUE(
            protocol,
            multiselect_->selectOneOf(
                std::vector<Protocol>{kDefaultEncryptionProtocol2}, conn, true))
        EXPECT_EQ(protocol, kDefaultEncryptionProtocol2);
        negotiated = true;
      });

  this->client([this](
                   outcome::result<std::shared_ptr<CapableConnection>> rconn) {
    EXPECT_OUTCOME_TRUE(conn, rconn);
    // first, we expect an exchange of opening messages
    negotiationOpeningsInitiator(*conn);

    // second, ls message will be sent to us; respond with a list of
    // encryption protocols we know
    negotiationLsInitiator(*conn,
                           std::vector<Protocol>{kDefaultEncryptionProtocol1,
                                                 kDefaultEncryptionProtocol2});

    // finally, we expect that the second of the protocols will be sent back
    // to us, as it is the common one; after that, we should send an ack
    negotiationProtocolsInitiator(*conn, kDefaultEncryptionProtocol2);
  });

  launchContext();
  EXPECT_TRUE(negotiated);
}

TEST_F(MultiselectTest, NegotiateAsListener) {
  auto negotiated = false;

  this->server(
      [this, &negotiated](
          outcome::result<std::shared_ptr<CapableConnection>> rconn) mutable {
        EXPECT_OUTCOME_TRUE(conn, rconn);
        EXPECT_OUTCOME_TRUE(
            protocol,
            multiselect_->selectOneOf(
                std::vector<Protocol>{kDefaultEncryptionProtocol2}, conn,
                false))
        EXPECT_EQ(protocol, kDefaultEncryptionProtocol2);
        negotiated = true;
      });

  this->client(
      [this](outcome::result<std::shared_ptr<CapableConnection>> rconn) {
        EXPECT_OUTCOME_TRUE(conn, rconn);
        // first, we expect an exchange of opening messages
        negotiationOpeningsListener(*conn);

        // second, send a protocol not supported by the other side and receive
        // an NA msg
        negotiationProtocolNaListener(*conn, kDefaultEncryptionProtocol1);

        // third, send ls and receive protocols, supported by the other side
        negotiationLsListener(
            *conn, std::vector<Protocol>{kDefaultEncryptionProtocol2});

        // fourth, send this protocol as our choice and receive an ack
        negotiationProtocolsListener(*conn, kDefaultEncryptionProtocol2);
      });

  launchContext();
  EXPECT_TRUE(negotiated);
}

/**
 * @given connection, over which we want to negotiate @and multiselect
 instance
 * over that connection @and encryption protocol, not supported by our side
 * @when negotiating about the protocol
 * @then the common protocol is not selected
 */
TEST_F(MultiselectTest, NegotiateFailure) {
  auto negotiated = false;

  this->server(
      [this, &negotiated](
          outcome::result<std::shared_ptr<CapableConnection>> rconn) mutable {
        EXPECT_OUTCOME_TRUE(conn, rconn);
        EXPECT_FALSE(multiselect_->selectOneOf(
            std::vector<Protocol>{kDefaultEncryptionProtocol1}, conn, true));
        negotiated = true;
      });

  this->client(
      [this](outcome::result<std::shared_ptr<CapableConnection>> rconn) {
        EXPECT_OUTCOME_TRUE(conn, rconn);
        negotiationOpeningsInitiator(*conn);
        // send a protocol, which is not supported by us
        negotiationLsInitiator(
            *conn, std::vector<Protocol>{kDefaultEncryptionProtocol2});
        return outcome::success();
      });

  launchContext();
  ASSERT_TRUE(negotiated);
}

/**
 * @given connection, over which we want to negotiate @and multiselect
 instance
 * over that connection @and no protocols, supported by our side
 * @when negotiating about the protocol
 * @then the common protocol is not selected
 */
TEST_F(MultiselectTest, NoProtocols) {
  std::shared_ptr<RawConnection> conn = std::make_shared<RawConnectionMock>();
  EXPECT_FALSE(multiselect_->selectOneOf(std::vector<Protocol>{}, conn, true));
}
