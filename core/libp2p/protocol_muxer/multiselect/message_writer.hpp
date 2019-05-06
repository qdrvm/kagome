/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MESSAGE_WRITER_HPP
#define KAGOME_MESSAGE_WRITER_HPP

#include <functional>
#include <memory>

#include <gsl/span>
#include "libp2p/peer/protocol.hpp"
#include "libp2p/protocol_muxer/multiselect/connection_state.hpp"

namespace libp2p::protocol_muxer {
  class Multiselect;

  /**
   * Sends messages of Multiselect format
   */
  class MessageWriter {
   public:
    /**
     * Send a message, signalizing about start of the negotiation
     * @param connection_state - state of the connection
     */
    static void sendOpeningMsg(
        std::shared_ptr<ConnectionState> connection_state);

    /**
     * Send a message, containing a protocol
     * @param protocol to be sent
     * @param connection_state - state of the connection
     */
    static void sendProtocolMsg(
        const peer::Protocol &protocol,
        std::shared_ptr<ConnectionState> connection_state);

    /**
     * Send a message, containing protocols
     * @param protocols to be sent
     * @param connection_state - state of the connection
     */
    static void sendProtocolsMsg(
        gsl::span<const peer::Protocol> protocols,
        std::shared_ptr<ConnectionState> connection_state);

    /**
     * Send a message, containing an ls
     * @param connection_state - state of the connection
     */
    static void sendLsMsg(std::shared_ptr<ConnectionState> connection_state);

    /**
     * Send a message, containing an na
     * @param connection_state - state of the connection
     */
    static void sendNaMsg(std::shared_ptr<ConnectionState> connection_state);

    /**
     * Send an ack message for the chosen protocol
     * @param connection_state - state of the connection
     * @param protocol - chosen protocol
     */
    static void sendProtocolAck(
        std::shared_ptr<ConnectionState> connection_state,
        const peer::Protocol &protocol);

   private:
    /**
     * Get a callback to be used in connection write functions
     * @param connection_state - state of the connection
     * @param success_status - status to be set after a successful write
     * @param error to be shown in case of write failure
     * @return lambda-callback for the write operation
     */
    static auto getWriteCallback(
        std::shared_ptr<ConnectionState> connection_state,
        ConnectionState::NegotiationStatus success_status,
        std::function<std::string(const std::error_code &ec)> error);
  };
}  // namespace libp2p::protocol_muxer

#endif  // KAGOME_MESSAGE_WRITER_HPP
