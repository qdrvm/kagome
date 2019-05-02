/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/protocol_muxer/multiselect/message_manager.hpp"

#include <string_view>
#include <vector>

#include <gtest/gtest.h>
#include <testutil/outcome.hpp>
#include "common/buffer.hpp"
#include "libp2p/peer/peer_id.hpp"
#include "libp2p/protocol_muxer/multiselect/multiselect.hpp"

using kagome::common::Buffer;
using libp2p::multi::Multihash;
using libp2p::multi::UVarint;
using libp2p::peer::Protocol;
using libp2p::protocol_muxer::MessageManager;
using libp2p::protocol_muxer::Multiselect;

using MessageType = MessageManager::MultiselectMessage::MessageType;

class MessageManagerTest : public ::testing::Test {
  static constexpr std::string_view kMultiselectHeaderProtocol =
      "/multistream-select/1.0.0";

 public:
  const std::vector<Protocol> kDefaultProtocols{
      "/plaintext/1.0.0", "/ipfs-dht/0.2.3", "/http/w3id.org/http/1.1"};
  static constexpr uint64_t kProtocolsListBytesSize = 60;

  const Buffer kOpeningMsg =
      Buffer{}
          .put(UVarint{kMultiselectHeaderProtocol.size() + 1}.toBytes())
          .put(kMultiselectHeaderProtocol)
          .put("\n");
  const Buffer kLsMsg = Buffer{}.put(UVarint{3}.toBytes()).put("ls\n");
  const Buffer kNaMsg = Buffer{}.put(UVarint{3}.toBytes()).put("na\n");
  const Buffer kProtocolMsg =
      Buffer{}
          .put(UVarint{kDefaultProtocols[0].size() + 1}.toBytes())
          .put(kDefaultProtocols[0])
          .put("\n");
  const Buffer kProtocolsMsg =
      Buffer{}
          .put(UVarint{3}.toBytes())
          .put(UVarint{kProtocolsListBytesSize}.toBytes())
          .put(UVarint{3}.toBytes())
          .put("\n")
          .put(UVarint{kDefaultProtocols[0].size() + 1}.toBytes())
          .put(kDefaultProtocols[0])
          .put("\n")
          .put(UVarint{kDefaultProtocols[1].size() + 1}.toBytes())
          .put(kDefaultProtocols[1])
          .put("\n")
          .put(UVarint{kDefaultProtocols[2].size() + 1}.toBytes())
          .put(kDefaultProtocols[2])
          .put("\n");
};

///**
// * @given message manager
// * @when getting an opening message from it
// * @then well-formed opening message is returned
// */
//TEST_F(MessageManagerTest, ComposeOpeningMessage) {
//  auto opening_msg = MessageManager::openingMsg();
//  ASSERT_EQ(opening_msg, kOpeningMsg);
//}
//
///**
// * @given message manager
// * @when getting an ls message from it
// * @then well-formed ls message is returned
// */
//TEST_F(MessageManagerTest, ComposeLsMessage) {
//  auto ls_msg = MessageManager::lsMsg();
//  ASSERT_EQ(ls_msg, kLsMsg);
//}
//
///**
// * @given message manager
// * @when getting an na message from it
// * @then well-formed na message is returned
// */
//TEST_F(MessageManagerTest, ComposeNaMessage) {
//  auto na_msg = MessageManager::naMsg();
//  ASSERT_EQ(na_msg, kNaMsg);
//}
//
///**
// * @given message manager @and protocol
// * @when getting a protocol message from it
// * @then well-formed protocol message is returned
// */
//TEST_F(MessageManagerTest, ComposeProtocolMessage) {
//  auto protocol_msg = MessageManager::protocolMsg(kDefaultProtocols[0]);
//  ASSERT_EQ(protocol_msg, kProtocolMsg);
//}
//
///**
// * @given message manager @and protocols
// * @when getting a protocols message from it
// * @then well-formed protocols message is returned
// */
//TEST_F(MessageManagerTest, ComposeProtocolsMessage) {
//  auto protocols_msg = MessageManager::protocolsMsg(kDefaultProtocols);
//  ASSERT_EQ(protocols_msg, kProtocolsMsg);
//}
//
///**
// * @given opening message
// * @when parsing it with a message manager
// * @then the message is parsed
// */
//TEST_F(MessageManagerTest, ParseOpeningMessage) {
//  EXPECT_OUTCOME_TRUE(opening_msg, MessageManager::parseMessage(kOpeningMsg))
//  ASSERT_EQ(opening_msg.type_, MessageType::OPENING);
//}
//
///**
// * @given ls message
// * @when parsing it with a message manager
// * @then the message is parsed
// */
//TEST_F(MessageManagerTest, ParseLsMessage) {
//  EXPECT_OUTCOME_TRUE(ls_msg, MessageManager::parseMessage(kLsMsg))
//  ASSERT_EQ(ls_msg.type_, MessageType::LS);
//}
//
///**
// * @given na message
// * @when parsing it with a message manager
// * @then the message is parsed
// */
//TEST_F(MessageManagerTest, ParseNaMessage) {
//  EXPECT_OUTCOME_TRUE(na_msg, MessageManager::parseMessage(kNaMsg))
//  ASSERT_EQ(na_msg.type_, MessageType::NA);
//}
//
///**
// * @given protocol message
// * @when parsing it with a message manager
// * @then the message is parsed
// */
//TEST_F(MessageManagerTest, ParseProtocolMessage) {
//  EXPECT_OUTCOME_TRUE(protocol_msg, MessageManager::parseMessage(kProtocolMsg))
//  ASSERT_EQ(protocol_msg.type_, MessageType::PROTOCOL);
//  ASSERT_EQ(protocol_msg.protocols_[0], kDefaultProtocols[0]);
//}
//
///**
// * @given protocols message
// * @when parsing it with a message manager
// * @then the message is parsed
// */
//TEST_F(MessageManagerTest, ParseProtocolsMessage) {
//  EXPECT_OUTCOME_TRUE(protocols_msg,
//                      MessageManager::parseMessage(kProtocolsMsg))
//  ASSERT_EQ(protocols_msg.type_, MessageType::PROTOCOLS);
//  ASSERT_EQ(protocols_msg.protocols_, kDefaultProtocols);
//}
