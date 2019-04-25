/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/protocol_muxer/multiselect/message_manager.hpp"

#include <string_view>

#include <boost/format.hpp>

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

  std::optional<MultiselectMessage> parseProtocolsMessage(
      const Buffer &buffer, size_t current_position,
      uint64_t current_line_length) {
    return {};
  }

  std::optional<MultiselectMessage> parseProtocolOrOpeningMessage(
      const std::string &msg) {
    // retrieve a peer id, which must lie after the '/p2p/' token, which is the
    // beginning of the string; peer id also ends with '/'
    auto string_pos = 5;  // length of /p2p/ prefix
    auto slash_pos = msg.find('/', string_pos);
    if (slash_pos == std::string::npos) {
      return {};
    }
    auto peer_id = msg.substr(string_pos, slash_pos - 1);

    // part of the string, which is left, is a Multistream protocol; check if it
    // is an opening protocol or a user one
    auto protocol_str = msg.substr(slash_pos);
    if (protocol_str == kMultiselectHeaderString) {
      return MultiselectMessage{MultiselectMessage::MessageType::OPENING};
    }

    auto protocol_res = Multistream::create(std::move(protocol_str), Buffer{});
    if (protocol_res.hasError()) {
      return {};
    }

    MultiselectMessage parsed_msg{MultiselectMessage::MessageType::PROTOCOL};
    parsed_msg.protocols_.push_back(protocol_res.getValue());
    return parsed_msg;
  }
}  // namespace

namespace libp2p::protocol_muxer {
  using kagome::common::Buffer;
  using MultiselectMessage = MessageManager::MultiselectMessage;

  std::optional<MultiselectMessage> MessageManager::parseMessage(
      const kagome::common::Buffer &buffer) const {
    static constexpr size_t kShortestMessageLength{4};

    static const std::string kLsMsg{"036C730A"};
    static const std::string kNaMsg{"036E610A"};
    static const std::string kP2pPrefix{"/p2p"};

    auto buffer_size = buffer.size();

    // shortest messages are LS, NA and (sometimes) a header for LS response
    if (buffer_size < kShortestMessageLength) {
      return {};
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
    // for LS response (or protocols message), protocol or opening messages; all
    // of them have a varint in the beginning, so start by retrieving it
    size_t current_position = 0;
    auto varint_opt = getVarint(buffer, current_position);
    if (!varint_opt) {
      return {};
    }

    // check that the buffer is not shorted than expected
    auto current_line_length = varint_opt->toUInt64();
    if (buffer_size < (varint_opt->size() + current_line_length)) {
      return {};
    }

    // make a choice between LS response or other two via checking a '/p2p'
    // token
    const auto *bytes = buffer.toBytes();
    std::string current_line(bytes + current_position,
                             bytes + current_position + current_line_length);

    if (!stringBeginsWith(current_line, kP2pPrefix)) {
      // it is LS response
      return parseProtocolsMessage(buffer, current_position,
                                   current_line_length);
    }

    // it is a protocol or opening message; both are one-liners, so check the
    // size of the buffer is as expected
    if (buffer_size != (varint_opt->size() + current_line_length)) {
      return {};
    }
    return parseProtocolOrOpeningMessage(current_line);
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
