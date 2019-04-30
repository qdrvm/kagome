/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/protocol_muxer/multiselect/message_manager.hpp"

#include <string_view>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/format.hpp>

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::protocol_muxer, MessageManager::ParseError,
                            e) {
  using Error = libp2p::protocol_muxer::MessageManager::ParseError;
  switch (e) {
    case Error::MSG_IS_TOO_SHORT:
      return "message size is less than a minimum one";
    case Error::VARINT_IS_EXPECTED:
      return "expected varint, but not found";
    case Error::MSG_LENGTH_IS_INCORRECT:
      return "incorrect message length";
    case Error::MSG_IS_ILL_FORMED:
      return "format of the message does not meet the protocol spec";
  }
  return "unknown error";
}

namespace {
  using kagome::common::Buffer;
  using libp2p::multi::UVarint;
  using MultiselectMessage =
      libp2p::protocol_muxer::MessageManager::MultiselectMessage;

  /// header of Multiselect protocol
  constexpr std::string_view kMultiselectHeaderString =
      "/multistream-select/0.3.0\n";

  /// string of ls message
  constexpr std::string_view kLsString = "ls\n";

  /// string of na message
  constexpr std::string_view kNaString = "na\n";

  /// ls message, ready to be sent
  const kagome::common::Buffer kLsMsg =
      kagome::common::Buffer{}
          .put(UVarint{kLsString.size()}.toBytes())
          .put(kLsString);

  /// na message, ready to be sent
  const kagome::common::Buffer kNaMsg =
      kagome::common::Buffer{}
          .put(UVarint{kNaString.size()}.toBytes())
          .put(kNaString);

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
      return ParseError::VARINT_IS_EXPECTED;
    }
    auto msg_length = msg_length_opt->toUInt64();
    if ((buffer.size() - msg_length_opt->size()) != msg_length) {
      return ParseError::MSG_LENGTH_IS_INCORRECT;
    }
    current_position += msg_length;

    assert(msg_length_opt->size() < current_position);             // NOLINT
    return std::string{buffer.toBytes() + msg_length_opt->size(),  // NOLINT
                       buffer.toBytes() + current_position};       // NOLINT
  }

  /**
   * Get a protocol from a string and check it meets specification requirements
   * @param msg, which must contain a protocol, ending with \n
   * @return pure protocol string or error
   */
  outcome::result<std::string_view> parseProtocol(std::string_view msg) {
    using ParseError = libp2p::protocol_muxer::MessageManager::ParseError;

    auto new_line_byte = msg.find('\n');
    if (new_line_byte != msg.size() - 1) {
      return ParseError::MSG_IS_ILL_FORMED;
    }

    return msg.substr(0, msg.size() - 1);
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
    // line in this case is going to be a protocol - check it meets the
    // requirements
    OUTCOME_TRY(current_line, lineToString(buffer, current_position));
    OUTCOME_TRY(protocol, parseProtocol(current_line));

    // if a parsed protocol is a Multiselect header, it's an opening message
    if (protocol == kMultiselectHeaderString) {
      return MultiselectMessage{MultiselectMessage::MessageType::OPENING};
    }

    auto msg = MultiselectMessage{MultiselectMessage::MessageType::PROTOCOL};
    msg.protocols_.emplace_back(std::string{protocol});
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
      return ParseError::VARINT_IS_EXPECTED;
    }
    auto line_length = line_length_opt->toUInt64();

    // next varint shows, how much bytes list of protocols take
    auto protocols_bytes_size = getVarint(buffer, current_position);
    if (!protocols_bytes_size) {
      return ParseError::VARINT_IS_EXPECTED;
    }
    if ((buffer.size() - line_length - line_length_opt->size())
        != protocols_bytes_size->toUInt64()) {
      return ParseError::MSG_LENGTH_IS_INCORRECT;
    }

    // next varint shows, how much protocols are expected in the message
    auto protocols_number = getVarint(buffer, current_position);
    if (!protocols_number) {
      return ParseError::VARINT_IS_EXPECTED;
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
      OUTCOME_TRY(current_line, lineToString(buffer, current_position));
      OUTCOME_TRY(protocol, parseProtocol(current_line));
      parsed_msg.protocols_.emplace_back(std::string{protocol});
    }

    return parsed_msg;
  }
}  // namespace

namespace libp2p::protocol_muxer {
  using kagome::common::Buffer;
  using MultiselectMessage = MessageManager::MultiselectMessage;

  outcome::result<MultiselectMessage> MessageManager::parseMessage(
      const kagome::common::Buffer &buffer) {
    static constexpr size_t kShortestMessageLength{4};

    static const std::string kLsMsgHex{"036C730A"};  // '3ls\n'
    static const std::string kNaMsgHex{"036E610A"};  // '3na\n'

    auto buffer_size = buffer.size();

    // shortest messages are LS, NA and (sometimes) a header for LS response
    if (buffer_size < kShortestMessageLength) {
      return ParseError::MSG_IS_TOO_SHORT;
    }

    // check the message against the constant ones
    if (buffer_size == kShortestMessageLength) {
      auto msg_hex = buffer.toHex();
      if (msg_hex == kLsMsgHex) {
        return MultiselectMessage{MultiselectMessage::MessageType::LS};
      }
      if (msg_hex == kNaMsgHex) {
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

    // parsing a protocol message was unsuccessful, try the other variant
    return parseProtocolsMessage(buffer);
  }

  Buffer MessageManager::openingMsg() {
    return kagome::common::Buffer{}
        .put(multi::UVarint{kMultiselectHeaderString.size()}.toBytes())
        .put(kMultiselectHeaderString);
  }

  Buffer MessageManager::lsMsg() {
    return kLsMsg;
  }

  Buffer MessageManager::naMsg() {
    return kNaMsg;
  }

  Buffer MessageManager::protocolMsg(const peer::Protocol &protocol) {
    return kagome::common::Buffer{}
        .put(multi::UVarint{protocol.size() + 1}.toBytes())
        .put(protocol)
        .put("\n");
  }

  Buffer MessageManager::protocolsMsg(
      gsl::span<const peer::Protocol> protocols) {
    // FIXME: naive approach, involving a tremendous amount of copies

    Buffer protocols_buffer{};
    for (const auto &protocol : protocols) {
      protocols_buffer.put(multi::UVarint{protocol.size() + 1}.toBytes())
          .put(protocol)
          .put("\n");
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
