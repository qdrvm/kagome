/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MESSAGE_MANAGER_HPP
#define KAGOME_MESSAGE_MANAGER_HPP

#include <memory>
#include <optional>
#include <vector>

#include <gsl/span>
#include <outcome/outcome.hpp>
#include "common/buffer.hpp"
#include "libp2p/multi/uvarint.hpp"
#include "libp2p/protocol_muxer/protocol_muxer.hpp"

namespace libp2p::protocol_muxer {
  /**
   * Creates and parses Multiselect messages to be sent over the network
   */
  class MessageManager {
   public:
    struct MultiselectMessage {
      enum class MessageType { OPENING, PROTOCOL, PROTOCOLS, LS, NA };

      /// type of the message
      MessageType type_;
      /// zero or more protocols in that message
      std::vector<std::string> protocols_{};
    };

    enum class ParseError {
      MSG_IS_TOO_SHORT = 1,
      VARINT_IS_EXPECTED,
      MSG_LENGTH_IS_INCORRECT,
      MSG_IS_ILL_FORMED
    };

    /**
     * Parse bytes into the message
     * @param bytes to be parsed
     * @return message, if parsing is successful, error otherwise
     */
    static outcome::result<MultiselectMessage> parseMessage(
        const kagome::common::Buffer &bytes);

    /**
     * Create an opening message
     * @return created message
     */
    static kagome::common::Buffer openingMsg();

    /**
     * Create a message with an ls command
     * @return created message
     */
    static kagome::common::Buffer lsMsg();

    /**
     * Create a message telling the protocol is not supported
     * @return created message
     */
    static kagome::common::Buffer naMsg();

    /**
     * Create a response message with a single protocol
     * @param protocol to be sent
     * @return created message
     */
    static kagome::common::Buffer protocolMsg(
        const ProtocolMuxer::Protocol &protocol);

    /**
     * Create a response message with a list of protocols
     * @param protocols to be sent
     * @return created message
     */
    static kagome::common::Buffer protocolsMsg(
        gsl::span<const ProtocolMuxer::Protocol> protocols);
  };
}  // namespace libp2p::protocol_muxer

OUTCOME_HPP_DECLARE_ERROR_2(libp2p::protocol_muxer, MessageManager::ParseError)

#endif  // KAGOME_MESSAGE_MANAGER_HPP
