/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/protocol_muxer/multiselect.hpp"

#include <gtest/gtest.h>
#include <testutil/outcome.hpp>
#include "core/libp2p/transport_fixture/transport_fixture.hpp"

using kagome::common::Buffer;
using libp2p::peer::Protocol;
using libp2p::protocol_muxer::MessageManager;
using libp2p::protocol_muxer::Multiselect;
using libp2p::protocol_muxer::ProtocolMuxer;
using libp2p::transport::Connection;

class MultiselectTest : public libp2p::testing::TransportFixture {
 public:
  void SetUp() override {
    libp2p::testing::TransportFixture::SetUp();

    // connection for "another" peer
    transport_listener_ =
        transport_->createListener([this](std::shared_ptr<Connection> conn) {
          this->other_peers_connection_ = std::move(conn);
        });
    defaultDial();
    launchContext();

    ASSERT_TRUE(connection_);
    ASSERT_TRUE(other_peers_connection_);
  }

  const Protocol kDefaultEncryptionProtocol1 = "/plaintext/1.0.0";
  const Protocol kDefaultEncryptionProtocol2 = "/plaintext/2.0.0";
  const Protocol kDefaultMultiplexerProtocol = "/mplex/6.7.0";
  const Protocol kDefaultStreamProocol = "/http/2.2.8";

  std::shared_ptr<Multiselect> multiselect_ = std::make_shared<Multiselect>();
  std::shared_ptr<Connection> other_peers_connection_;

  void negotiationOpenings() {
    auto expected_opening_msg =
        std::make_shared<Buffer>(MessageManager::openingMsg());
    auto opening_msg_buf =
        std::make_shared<Buffer>(expected_opening_msg->size(), 0);
    other_peers_connection_->asyncRead(
        boost::asio::buffer(opening_msg_buf->toVector()),
        expected_opening_msg->size(),
        [this, expected_opening_msg, opening_msg_buf](const std::error_code &ec,
                                                      size_t n) {
          checkIOSuccess(ec, n, expected_opening_msg->size());
          ASSERT_EQ(*opening_msg_buf, *expected_opening_msg);

          other_peers_connection_->asyncWrite(
              boost::asio::buffer(expected_opening_msg->toVector()),
              [expected_opening_msg](const std::error_code &ec, size_t n) {
                checkIOSuccess(ec, n, expected_opening_msg->size());
              });
        });
  }

  void negotiationLs(gsl::span<const Protocol> protos_to_send) {
    auto expected_ls_msg = std::make_shared<Buffer>(MessageManager::lsMsg());
    auto ls_msg_buf = std::make_shared<Buffer>(expected_ls_msg->size(), 0);
    auto encryption_protocols_msg =
        std::make_shared<Buffer>(MessageManager::protocolsMsg(protos_to_send));
    other_peers_connection_->asyncRead(
        boost::asio::buffer(ls_msg_buf->toVector()), expected_ls_msg->size(),
        [this, expected_ls_msg, ls_msg_buf, encryption_protocols_msg](
            const std::error_code &ec, size_t n) {
          checkIOSuccess(ec, n, expected_ls_msg->size());
          ASSERT_EQ(*ls_msg_buf, *expected_ls_msg);

          other_peers_connection_->asyncWrite(
              boost::asio::buffer(encryption_protocols_msg->toVector()),
              [encryption_protocols_msg](const std::error_code &ec, size_t n) {
                checkIOSuccess(ec, n, encryption_protocols_msg->size());
              });
        });
  }

  void negotiationNa() {
    auto expected_na_msg = std::make_shared<Buffer>(MessageManager::naMsg());
    auto na_msg_buf = std::make_shared<Buffer>(expected_na_msg->size(), 0);
    other_peers_connection_->asyncRead(
        boost::asio::buffer(na_msg_buf->toVector()), expected_na_msg->size(),
        [expected_na_msg, na_msg_buf](const std::error_code &ec, size_t n) {
          checkIOSuccess(ec, n, expected_na_msg->size());
          ASSERT_EQ(*na_msg_buf, *expected_na_msg);
        });
  }

  void negotiationProtocols(const Protocol &expected_protocol) {
    auto expected_proto_msg = std::make_shared<Buffer>(
        MessageManager::protocolMsg(expected_protocol));
    auto proto_msg_buf =
        std::make_shared<Buffer>(expected_proto_msg->size(), 0);
    other_peers_connection_->asyncRead(
        boost::asio::buffer(proto_msg_buf->toVector()),
        expected_proto_msg->size(),
        [this, expected_proto_msg, proto_msg_buf](const std::error_code &ec,
                                                  size_t n) {
          checkIOSuccess(ec, n, expected_proto_msg->size());
          ASSERT_EQ(*proto_msg_buf, *expected_proto_msg);

          other_peers_connection_->asyncWrite(
              boost::asio::buffer(expected_proto_msg->toVector()),
              [expected_proto_msg](const std::error_code &ec, size_t n) {
                checkIOSuccess(ec, n, expected_proto_msg->size());
              });
        });
  }
};

/**
 * @given connection, over which we want to negotiate, @and multiselect instance
 * over that connection @and encryption protocol, supported by both sides
 * @when negotiating about the protocols
 * @then the two common protocols are selected
 */
TEST_F(MultiselectTest, NegotiateEncryption) {
  multiselect_->addEncryptionProtocol(kDefaultEncryptionProtocol2);

  // create a success handler to be called, when a negotiation is finished
  multiselect_->negotiateEncryption(
      connection_, [this](outcome::result<Protocol> protocol_res) {
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

TEST_F(MultiselectTest, NegotiateMultiplexer) {
  multiselect_->addMultiplexerProtocol(kDefaultMultiplexerProtocol);

  // create a success handler to be called, when a negotiation is finished
  multiselect_->negotiateMultiplexer(
      connection_, [this](outcome::result<Protocol> protocol_res) {
        EXPECT_OUTCOME_TRUE(protocol, protocol_res)
        EXPECT_EQ(protocol, kDefaultMultiplexerProtocol);
      });

  negotiationOpenings();
  negotiationLs(std::vector<Protocol>{kDefaultMultiplexerProtocol});
  negotiationProtocols(kDefaultMultiplexerProtocol);

  launchContext();
}

TEST_F(MultiselectTest, NegotiateStream) {}

TEST_F(MultiselectTest, NegotiateFailure) {
  multiselect_->addEncryptionProtocol(kDefaultEncryptionProtocol1);

  // create a failure handler to be called, when a negotiation is finished
  multiselect_->negotiateEncryption(connection_,
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

TEST_F(MultiselectTest, NoProtocols) {
  // create a failure handler, which is going to be called immediately
  multiselect_->negotiateEncryption(connection_,
                                    [](outcome::result<Protocol> protocol_res) {
                                      EXPECT_FALSE(protocol_res);
                                    });
}
