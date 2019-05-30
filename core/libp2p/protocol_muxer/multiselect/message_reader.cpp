/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/protocol_muxer/multiselect/message_reader.hpp"

#include <optional>

#include "libp2p/multi/uvarint.hpp"
#include "libp2p/protocol_muxer/multiselect/message_manager.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::protocol_muxer, MessageReader::ReaderError,
                            e) {
  using E = libp2p::protocol_muxer::MessageReader::ReaderError;
  switch (e) {
    case E::VARINT_EXPECTED:
      return "expected a varint in the message";
    case E::MSG_TOO_SHORT:
      return "varint in the beginning of the message is tells the message is "
             "too short";
  }
  return "unknown error";
}

namespace {
  using libp2p::multi::UVarint;

  /**
   * Read next varint from the connection
   * @param connection to read from
   * @return varint, which was read
   */
  outcome::result<UVarint> readNextVarint(
      const std::shared_ptr<libp2p::basic::ReadWriteCloser> &connection) {
    // we don't know a length of the varint, so gather it byte-by-byte
    std::vector<uint8_t> varint_bytes{};
    std::optional<UVarint> varint{std::nullopt};
    while (!varint) {
      OUTCOME_TRY(next_byte, connection->read(1));
      varint_bytes.push_back(next_byte[0]);
      varint = UVarint::create(varint_bytes);
    }
    return *varint;
  }
}  // namespace

namespace libp2p::protocol_muxer {
  using kagome::common::Buffer;

  outcome::result<MessageManager::MultiselectMessage>
  MessageReader::readNextMessage(
      const std::shared_ptr<basic::ReadWriteCloser> &connection) {
    // '/tls/1.3.0\n' - the shortest protocol, which could be found
    static constexpr size_t kShortestProtocolLength = 11;

    // const messages without first varint byte
    static constexpr int64_t kShortestMessageLength{3};

    // start with reading a varint, which must be at the beginning of each
    // message
    OUTCOME_TRY(varint, readNextVarint(connection));
    if (varint.toUInt64() < kShortestMessageLength) {
      return ReaderError::MSG_TOO_SHORT;
    }

    // that varint tells us, how much bytes to read next; do it
    OUTCOME_TRY(msg, connection->read(varint.toUInt64()));

    // firstly, try to match the message against constant messages
    if (auto const_msg_opt = MessageManager::parseConstantMsg(msg);
        const_msg_opt) {
      return *const_msg_opt;
    }

    // assume that protocols header - two varints + '\n' - cannot be longer or
    // equal to the shortest protocol length; varints should be very big for it
    // to happen; parse depending on the length of the current string
    if (msg.size() < kShortestProtocolLength) {
      // parse the protocols header, read protocols message and parse it
      OUTCOME_TRY(protocols_header, MessageManager::parseProtocolsHeader(msg));
      OUTCOME_TRY(protocols_msg,
                  connection->read(protocols_header.size_of_protocols));
      return MessageManager::parseProtocols(
          protocols_msg, protocols_header.number_of_protocols);
    }

    return MessageManager::parseProtocol(msg);
  }

}  // namespace libp2p::protocol_muxer
