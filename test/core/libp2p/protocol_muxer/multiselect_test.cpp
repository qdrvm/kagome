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
//  void SetUp() override {
//    libp2p::testing::TransportFixture::SetUp();
//
//    // connection for "another" peer
//    transport_listener_ =
//        transport_->createListener([this](std::shared_ptr<Connection> conn) {
//          this->other_peers_connection_ = std::move(conn);
//        });
//    defaultDial();
//    launchContext();
//
//    ASSERT_TRUE(connection_);
//    ASSERT_TRUE(other_peers_connection_);
//  }
//
//  const Protocol kDefaultEncryptionProtocol1 = "/plaintext/1.0.0";
//  const Protocol kDefaultEncryptionProtocol2 = "/plaintext/2.0.0";
//  const Protocol kDefaultMultiplexerProtocol = "/mplex/6.7.0";
//  const Protocol kDefaultStreamProocol = "/http/2.2.8";
//
//  Multiselect multiselect_;
//
//  std::shared_ptr<Connection> other_peers_connection_;
};
//
///**
// * @given connection, over which we want to negotiate, @and multiselect instance
// * over that connection @and two protocols, supported by both sides
// * @when negotiating about the protocols
// * @then the two common protocols are selected
// */
//TEST_F(MultiselectTest, NegotiateSuccess) {
//  multiselect_.addEncryptionProtocol(kDefaultEncryptionProtocol1);
//  multiselect_.addMultiplexerProtocol(kDefaultMultiplexerProtocol);
//  multiselect_.addStreamProtocol(kDefaultStreamProocol);
//
//  // create a success handler to be called, when a negotiation is finished
//  multiselect_.negotiateClient(
//      connection_,
//      [this](
//          outcome::result<ProtocolMuxer::NegotiatedProtocols> protocols_res) {
//        EXPECT_OUTCOME_TRUE(protocols, protocols_res)
//        EXPECT_EQ(protocols.encryption_protocol_, kDefaultEncryptionProtocol1);
//        EXPECT_EQ(protocols.multiplexer_protocol_, kDefaultMultiplexerProtocol);
//        EXPECT_EQ(protocols.initial_stream_protocol_, kDefaultStreamProocol);
//      });
//
//  // first, we expect an opening message to be sent; respond with it as well
//  auto expected_opening_msg = MessageManager::openingMsg();
//  auto opening_msg_buf = std::make_shared<Buffer>();
//  other_peers_connection_->asyncRead(
//      boost::asio::buffer(opening_msg_buf->toVector()),
//      expected_opening_msg.size(),
//      [this, &expected_opening_msg, opening_msg_buf](const std::error_code &ec,
//                                                     size_t n) {
//        checkIOSuccess(ec, n, expected_opening_msg.size());
//        ASSERT_EQ(*opening_msg_buf, expected_opening_msg);
//
//        other_peers_connection_->asyncWrite(
//            boost::asio::buffer(expected_opening_msg.toVector()),
//            [&expected_opening_msg](const std::error_code &ec, size_t n) {
//              checkIOSuccess(ec, n, expected_opening_msg.size());
//            });
//      });
//
//  // second, we want to test an ability of sending ls message
//  auto expected_ls_msg = MessageManager::lsMsg();
//  auto ls_msg_buf = std::make_shared<Buffer>();
//  auto encryption_protocols_msg =
//      MessageManager::protocolsMsg(std::vector<Protocol>{
//          kDefaultEncryptionProtocol1, kDefaultEncryptionProtocol2});
//  other_peers_connection_->asyncRead(
//      boost::asio::buffer(ls_msg_buf->toVector()), expected_ls_msg.size(),
//      [this, &expected_ls_msg, ls_msg_buf, &encryption_protocols_msg](
//          const std::error_code &ec, size_t n) {
//        checkIOSuccess(ec, n, expected_ls_msg.size());
//        ASSERT_EQ(*ls_msg_buf, expected_ls_msg);
//
//        connection_->asyncWrite(
//            boost::asio::buffer(encryption_protocols_msg.toVector()),
//            [&encryption_protocols_msg](const std::error_code &ec, size_t n) {
//              checkIOSuccess(ec, n, encryption_protocols_msg.size());
//            });
//      });
//
//
//}
//
//TEST_F(MultiselectTest, NegotiateFailure) {}
//
//TEST_F(MultiselectTest, NoProtocols) {}
