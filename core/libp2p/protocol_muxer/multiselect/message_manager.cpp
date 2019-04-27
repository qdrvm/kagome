/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/protocol_muxer/multiselect/message_manager.hpp"

#include <string_view>

#include <boost/format.hpp>

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::protocol_muxer, MessageManager::ParseError,
                            e) {
  using Error = libp2p::protocol_muxer::MessageManager::ParseError;
  switch (e) {
    case Error::MSG_IS_TOO_SHORT:
      return "message size is less, than a minimum one";
    case Error::VARINT_WAS_EXPECTED:
      return "cannot find a varint, where it was supposed to be";
    case Error::MSG_LENGTH_IS_INCORRECT:
      return "length of the message is not as specified in varints inside";
    case Error::PEER_ID_WAS_EXPECTED:
      return "cannot find a peer id, where it was supposed to be";
    case Error::NO_P2P_PREFIX:
      return "cannot find /p2p prefix, where it was supposed to be";
  }
  return "uknown error";
}

namespace {
  using kagome::common::Buffer;
  using libp2p::multi::Multistream;
  using libp2p::multi::UVarint;
  using MultiselectMessage =
      libp2p::protocol_muxer::MessageManager::MultiselectMessage;

  /// produces protocol string to be sent: '/p2p/PEER_ID/PROTOCOL\n'
  static boost::format kProtocolTemplate = boost::format("/p2p/%1%2\n");

  /// header of Multiselect protocol
  static constexpr std::string_view kMultiselectHeaderString =
      "/multistream-select/0.3.0";

  /**
   * Retrieve a varint from a bytes buffer
   * @param buffer to be seeked
   * @param pos - position, from which the retrieval should start; after the
   * function execution it is set to the position AFTER the found varint
   * @return varint, if it was retrieved; none otherwise
   */
  std::optional<UVarint> getVarint(const Buffer &buffer, size_t &pos) {
    std::vector<uint8_t> varint_candidate;
    uint8_t current_byte;
    while (pos < buffer.size()) {
      current_byte = buffer[pos];
      varint_candidate.push_back(current_byte);
      ++pos;
      // last varint in the array has a 0 most significant bit
      if ((current_byte << 7) == 0) {
        return UVarint{varint_candidate};
      }
    }
    return {};
  }

  /**
   * Check that a string begins with a specified substring
   * @param string to be checked
   * @param substring to be found
   * @return true, if the string begins with a substring, false otherwise
   */
  bool stringBeginsWith(std::string_view string, std::string_view substring) {
    if (string.size() < substring.size()) {
      return false;
    }
    for (size_t i = 0; i < substring.size(); ++i) {
      if (string[i] != substring[i]) {
        return false;
      }
    }
    return true;
  }

  /**
   * Retrieve a line from a buffer, starting from the specified position
   * @param buffer, from which the line is to be retrieved
   * @param current_position, from which to get the line; after the execution is
   * set to a position after the line
   * @return line in case of success, error otherwise
   */
  outcome::result<std::string> lineToString(const Buffer &buffer,
                                            size_t &current_position) {
    using ParseError = libp2p::protocol_muxer::MessageManager::ParseError;

    // firstly, a varint, showing length of this line (and thus a whole message)
    // without itself
    auto msg_length_opt = getVarint(buffer, current_position);
    if (!msg_length_opt) {
      return ParseError::VARINT_WAS_EXPECTED;
    }
    auto msg_length = msg_length_opt->toUInt64();
    if ((buffer.size() - msg_length_opt->size()) != msg_length) {
      return ParseError::MSG_LENGTH_IS_INCORRECT;
    }

    // retrieve a line, which is supposed to contain peer id and protocol, and
    // parse it
    const auto *bytes = buffer.toBytes();
    current_position += msg_length;
    return std::string(bytes + current_position - msg_length,
                       bytes + current_position);
  }

  /**
   * Parse part of message, which contains a protocol inside
   * @param msg to be parsed
   * @return pair {PeerId, Protocol} in case of success, error otherwise
   */
  outcome::result<std::pair<std::string, std::string>> parsePeerIdAndProtocol(
      const std::string &msg) {
    using ParseError = libp2p::protocol_muxer::MessageManager::ParseError;
    static const std::string kP2pPrefix{"/p2p"};

    // the message must begin with a prefix
    if (!stringBeginsWith(msg, kP2pPrefix)) {
      return ParseError::NO_P2P_PREFIX;
    }

    // retrieve a peer id, which must lie after the '/p2p/' token, which is the
    // beginning of the string; peer id also ends with '/'
    auto string_pos = 5;  // length of /p2p/ prefix
    auto slash_pos = msg.find('/', string_pos);
    if (slash_pos == std::string::npos) {
      return ParseError::PEER_ID_WAS_EXPECTED;
    }

    return std::make_pair(msg.substr(string_pos, slash_pos - 1),  // peer_id
                          msg.substr(slash_pos));                 // protocol
  }

  /**
   * Parse a buffer, starting from the specified position, to get a message with
   * a single protocol (or an opening one)
   * @param buffer to be parsed
   * @param current_position, from which to start parsing; after the execution
   * is set after the parsed part
   * @return message in case of success, error otherwise
   */
  outcome::result<MultiselectMessage> parseProtocolMessage(
      const Buffer &buffer, size_t &current_position) {
    auto current_line = lineToString(buffer, current_position);
    if (!current_line) {
      return current_line.error();
    }
    auto peer_id_proto = parsePeerIdAndProtocol(current_line.value());
    if (!peer_id_proto) {
      return peer_id_proto.error();
    }

    // if a parsed protocol is a Multiselect header, it's an opening message
    if (peer_id_proto.value().second == kMultiselectHeaderString) {
      return MultiselectMessage{MultiselectMessage::MessageType::OPENING};
    }

    auto msg = MultiselectMessage{MultiselectMessage::MessageType::PROTOCOL};
    msg.protocols_.push_back(std::move(peer_id_proto.value().second));
    return msg;
  }

  /**
   * Parse message, which contains several protocols
   * @param buffer with the message
   * @return multiselect message, if parse is successful, error otherwise
   */
  outcome::result<MultiselectMessage> parseProtocolsMessage(
      const Buffer &buffer) {
    using ParseError = libp2p::protocol_muxer::MessageManager::ParseError;

    size_t current_position = 0;
    // firstly, a varint, showing length of this line without itself
    auto line_length_opt = getVarint(buffer, current_position);
    if (!line_length_opt) {
      return ParseError::VARINT_WAS_EXPECTED;
    }
    auto line_length = line_length_opt->toUInt64();

    // next varint shows, how much bytes list of protocols take
    auto protocols_bytes_size = getVarint(buffer, current_position);
    if (!protocols_bytes_size) {
      return ParseError::VARINT_WAS_EXPECTED;
    }
    if ((buffer.size() - line_length - line_length_opt->size())
        != protocols_bytes_size->toUInt64()) {
      return ParseError::MSG_LENGTH_IS_INCORRECT;
    }

    // next varint shows, how much protocols are expected in the message
    auto protocols_number = getVarint(buffer, current_position);
    if (!protocols_number) {
      return ParseError::VARINT_WAS_EXPECTED;
    }
    // increment the position to skip '\n' byte
    ++current_position;

    // finally, check that length of the first line was as expected (+1 for \n
    // byte)
    if ((protocols_bytes_size->size() + protocols_number->size() + 1)
        != line_length) {
      return ParseError::MSG_LENGTH_IS_INCORRECT;
    }

    // parse protocols, which are after the header
    MultiselectMessage parsed_msg{MultiselectMessage::MessageType::PROTOCOLS};
    for (uint64_t i = 0; i < protocols_number->toUInt64(); ++i) {
      auto current_line = lineToString(buffer, current_position);
      if (!current_line) {
        return current_line.error();
      }
      auto peer_id_proto = parsePeerIdAndProtocol(current_line.value());
      if (!peer_id_proto) {
        return peer_id_proto.error();
      }
      parsed_msg.protocols_.push_back(std::move(peer_id_proto.value().second));
    }

    return parsed_msg;
  }
}  // namespace

namespace libp2p::protocol_muxer {
  using kagome::common::Buffer;
  using MultiselectMessage = MessageManager::MultiselectMessage;

  outcome::result<MultiselectMessage> MessageManager::parseMessage(
      const kagome::common::Buffer &buffer) const {
    static constexpr size_t kShortestMessageLength{4};

    static const std::string kLsMsg{"036C730A"};
    static const std::string kNaMsg{"036E610A"};

    auto buffer_size = buffer.size();

    // shortest messages are LS, NA and (sometimes) a header for LS response
    if (buffer_size < kShortestMessageLength) {
      return ParseError::MSG_IS_TOO_SHORT;
    }

    // check the message against the constant ones
    if (buffer_size == kShortestMessageLength) {
      auto msg_hex = buffer.toHex();
      if (msg_hex == kLsMsg) {
        return MultiselectMessage{MultiselectMessage::MessageType::LS};
      }
      if (msg_hex == kNaMsg) {
        return MultiselectMessage{MultiselectMessage::MessageType::NA};
      }
    }

    // now, we can forget about LS and NA messages and focus on parsing a header
    // for LS response (or protocols message), protocol or opening messages; try
    // to parse bytes as a single-protocol message
    size_t current_position = 0;
    auto protocol_msg_res = parseProtocolMessage(buffer, current_position);
    if (protocol_msg_res
        || (!protocol_msg_res
            && protocol_msg_res.error()
                != ParseError::MSG_LENGTH_IS_INCORRECT)) {
      // this result either contains a value or a principal error
      return protocol_msg_res;
    }

    // parsing a protocol message was unsuccesful, try the other variant
    return parseProtocolsMessage(buffer);
  }

  Buffer MessageManager::openingMsg(const peer::PeerId &peer_id) const {
    auto opening_string = (kProtocolTemplate % peer_id.getPeerId().toHex()
                           % kMultiselectHeaderString)
                              .str();
    return kagome::common::Buffer{}
        .put(multi::UVarint{opening_string.size()}.toBytes())
        .put(opening_string);
  }

  Buffer MessageManager::lsMsg() const {
    return ls_msg_;
  }

  Buffer MessageManager::naMsg() const {
    return na_msg_;
  }

  Buffer MessageManager::protocolMsg(const peer::PeerId &peer_id,
                                     const multi::Multistream &protocol) const {
    // FIXME: string copy; can be fixed by calculating the size in advance, but
    // will look very ugly
    auto protocol_string = (kProtocolTemplate % peer_id.getPeerId().toHex()
                            % protocol.getProtocolPath())
                               .str();
    return kagome::common::Buffer{}
        .put(multi::UVarint{protocol_string.size()}.toBytes())
        .put(protocol_string);
  }

  Buffer MessageManager::protocolsMsg(
      const peer::PeerId &peer_id,
      gsl::span<const multi::Multistream> protocols) const {
    // FIXME: naive approach, involving a tremendous amount of copies

    Buffer protocols_buffer{};
    for (const auto &protocol : protocols) {
      auto protocol_string = (kProtocolTemplate % peer_id.getPeerId().toHex()
                              % protocol.getProtocolPath())
                                 .str();
      protocols_buffer.put(multi::UVarint{protocol_string.size()}.toBytes())
          .put(protocol_string);
    }

    auto header_buffer =
        Buffer{}
            .put(multi::UVarint{protocols_buffer.size()}.toBytes())
            .put(multi::UVarint{static_cast<uint64_t>(protocols.size())}
                     .toBytes())
            .put("\n");

    return Buffer{}
        .put(multi::UVarint{header_buffer.size()}.toBytes())
        .putBuffer(header_buffer)
        .putBuffer(protocols_buffer);
  }
}  // namespace libp2p::protocol_muxer
