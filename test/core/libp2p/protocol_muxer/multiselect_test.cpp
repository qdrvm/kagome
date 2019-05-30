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
using libp2p::connection::RawConnection;
using libp2p::connection::RawConnectionMock;
using libp2p::connection::SecureConnection;
using libp2p::connection::Stream;
using libp2p::peer::Protocol;
using libp2p::protocol_muxer::MessageManager;
using libp2p::protocol_muxer::Multiselect;
using libp2p::testing::TransportFixture;

namespace std {
  std::ostream &operator<<(std::ostream &s,
                           const std::vector<unsigned char> &v) {
    s << std::string(v.begin(), v.end()) << "\n";
    return s;
  }

  std::ostream &operator<<(std::ostream &s,
                           const libp2p::multi::Multiaddress &m) {
    s << m.getStringAddress() << "\n";
    return s;
  }
}  // namespace std

class MultiselectTest : public TransportFixture {
 public:
  const Protocol kDefaultEncryptionProtocol1 = "/plaintext/1.0.0";
  const Protocol kDefaultEncryptionProtocol2 = "/plaintext/2.0.0";
  const Protocol kDefaultMultiplexerProtocol = "/mplex/6.7.0";
  const Protocol kDefaultStreamProtocol = "/http/2.2.8";

  std::shared_ptr<Multiselect> multiselect_ = std::make_shared<Multiselect>();

  /**
   * Exchange opening messages with the other side
   */
  static void negotiationOpenings(ReadWriteCloser &conn) {
    auto expected_opening_msg = MessageManager::openingMsg();

    EXPECT_OUTCOME_TRUE(read_msg, conn.read(expected_opening_msg.size()))
    ASSERT_EQ(read_msg, expected_opening_msg.toVector());

    EXPECT_OUTCOME_TRUE(written_bytes,
                        conn.write(expected_opening_msg.toVector()))
    ASSERT_EQ(written_bytes, expected_opening_msg.size());
  }

  /**
   * Expect to receive an LS msg and respond with a list of protocols
   * @param conn - connection, over which the negotiation goes
   * @param protos_to_send - protocols, which are going to be sent
   */
  static void negotiationLs(ReadWriteCloser &conn,
                            gsl::span<const Protocol> protos_to_send) {
    auto expected_ls_msg = MessageManager::lsMsg();
    auto protocols_msg = MessageManager::protocolsMsg(protos_to_send);

    EXPECT_OUTCOME_TRUE(read_msg, conn.read(expected_ls_msg.size()))
    EXPECT_EQ(read_msg, expected_ls_msg.toVector());

    EXPECT_OUTCOME_TRUE(written_bytes, conn.write(protocols_msg.toVector()))
    ASSERT_EQ(written_bytes, protocols_msg.size());
  }

  /**
   * Expect to receive a protocol msg and respond with the same message as an
   * acknowledgement
   * @param conn - connection, over which the negotiation goes
   * @param expected_protocol - protocol, which is expected to be received and
   * sent back
   */
  static void negotiationProtocols(ReadWriteCloser &conn,
                                   const Protocol &expected_protocol) {
    auto expected_proto_msg = MessageManager::protocolMsg(expected_protocol);

    EXPECT_OUTCOME_TRUE(read_msg, conn.read(expected_proto_msg.size()))
    EXPECT_EQ(read_msg, expected_proto_msg.toVector());

    EXPECT_OUTCOME_TRUE(written_bytes,
                        conn.write(expected_proto_msg.toVector()))
    ASSERT_EQ(written_bytes, expected_proto_msg.size());
  }
};

/**
 * @given connection, over which we want to negotiate @and multiselect instance
 * over that connection @and encryption protocol, supported by both sides
 * @when negotiating about the protocol
 * @then the common protocol is selected
 */
TEST_F(MultiselectTest, NegotiateEncryption) {
  auto negotiated = false;
  multiselect_->addEncryptionProtocol(kDefaultEncryptionProtocol2);

  this->server(
      [this, &negotiated](std::shared_ptr<RawConnection> conn) mutable {
        EXPECT_OUTCOME_TRUE(protocol, multiselect_->negotiateEncryption(conn))
        EXPECT_EQ(protocol, kDefaultEncryptionProtocol2);
        negotiated = true;
        return outcome::success();
      },
      [](auto &&) { FAIL() << "cannot create server"; });

  this->client(
      [this](std::shared_ptr<RawConnection> conn) {
        // first, we expect an exchange of opening messages
        negotiationOpenings(*conn);

        // second, ls message will be sent to us; respond with a list of
        // encryption protocols we know
        negotiationLs(*conn,
                      std::vector<Protocol>{kDefaultEncryptionProtocol1,
                                            kDefaultEncryptionProtocol2});

        // finally, we expect that the second of the protocols will be sent back
        // to us, as it is the common one; after that, we should send an ack
        negotiationProtocols(*conn, kDefaultEncryptionProtocol2);
        return outcome::success();
      },
      [](auto &&) { FAIL() << "cannot create client"; });

  launchContext();
  EXPECT_TRUE(negotiated);
}

/**
 * @given connection, over which we want to negotiate @and multiselect
 instance
 * over that connection @and mutiplexer protocol, supported by both sides
 * @when negotiating about the protocol
 * @then the common protocol is selected
 */
TEST_F(MultiselectTest, NegotiateMultiplexer) {
  auto negotiated = false;
  multiselect_->addMultiplexerProtocol(kDefaultMultiplexerProtocol);

  this->server(
      [this, &negotiated](std::shared_ptr<RawConnection> conn) mutable {
        EXPECT_OUTCOME_TRUE(
            protocol,
            multiselect_->negotiateMultiplexer(
                std::static_pointer_cast<SecureConnection>(conn)))
        EXPECT_EQ(protocol, kDefaultMultiplexerProtocol);
        negotiated = true;
        return outcome::success();
      },
      [](auto &&) { FAIL() << "cannot create server"; });

  this->client(
      [this](std::shared_ptr<RawConnection> conn) {
        negotiationOpenings(*conn);
        negotiationLs(*conn,
                      std::vector<Protocol>{kDefaultMultiplexerProtocol});
        negotiationProtocols(*conn, kDefaultMultiplexerProtocol);
        return outcome::success();
      },
      [](auto &&) { FAIL() << "cannot create client"; });

  launchContext();
  EXPECT_TRUE(negotiated);
}

/**
 * @given stream, over which we want to negotiate @and multiselect instance
 * over that connection @and stream protocol, supported by both sides
 * @when negotiating about the protocol
 * @then the common protocol is selected
 */
TEST_F(MultiselectTest, NegotiateStream) {
  auto negotiated = false;
  multiselect_->addStreamProtocol(kDefaultStreamProtocol);

  this->server(
      [this, &negotiated](std::shared_ptr<RawConnection> conn) mutable {
        EXPECT_OUTCOME_TRUE(
            protocol,
            multiselect_->negotiateAppProtocol(std::static_pointer_cast<Stream>(
                std::static_pointer_cast<ReadWriteCloser>(conn))))
        EXPECT_EQ(protocol, kDefaultStreamProtocol);
        negotiated = true;
        return outcome::success();
      },
      [](auto &&) { FAIL() << "cannot create server"; });

  this->client(
      [this](std::shared_ptr<RawConnection> conn) {
        negotiationOpenings(*conn);
        negotiationLs(*conn, std::vector<Protocol>{kDefaultStreamProtocol});
        negotiationProtocols(*conn, kDefaultStreamProtocol);
        return outcome::success();
      },
      [](auto &&) { FAIL() << "cannot create client"; });

  launchContext();
  ASSERT_TRUE(negotiated);
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
  multiselect_->addEncryptionProtocol(kDefaultEncryptionProtocol1);

  this->server(
      [this, &negotiated](std::shared_ptr<RawConnection> conn) mutable {
        EXPECT_FALSE(multiselect_->negotiateEncryption(conn));
        negotiated = true;
        return outcome::success();
      },
      [](auto &&) { FAIL() << "cannot create server"; });

  this->client(
      [this](std::shared_ptr<RawConnection> conn) {
        negotiationOpenings(*conn);
        // send a protocol, which is not supported by us
        negotiationLs(*conn,
                      std::vector<Protocol>{kDefaultEncryptionProtocol2});
        return outcome::success();
      },
      [](auto &&) { FAIL() << "cannot create client"; });

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
  EXPECT_FALSE(multiselect_->negotiateEncryption(conn));
}
