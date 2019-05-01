/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MULTISELECT_IMPL_HPP
#define KAGOME_MULTISELECT_IMPL_HPP

#include <memory>
#include <string>
#include <vector>

#include <gsl/span>
#include "common/logger.hpp"
#include "libp2p/protocol_muxer/multiselect/stream_state.hpp"
#include "libp2p/protocol_muxer/protocol_muxer.hpp"

namespace libp2p::protocol_muxer {
  /**
   * Implementation of a protocol muxer. Read more
   * https://github.com/multiformats/multistream-select
   */
  class Multiselect : public ProtocolMuxer,
                      public std::enable_shared_from_this<Multiselect> {
   public:
    /**
     * Create a Multiselect instance
     * @param logger to write debug messages to
     */
    explicit Multiselect(kagome::common::Logger logger =
                             kagome::common::createLogger("Multiselect"));

    Multiselect(const Multiselect &other) = delete;
    Multiselect &operator=(const Multiselect &other) = delete;
    Multiselect(Multiselect &&other) noexcept = default;
    Multiselect &operator=(Multiselect &&other) noexcept = default;

    ~Multiselect() override = default;

    void addEncryptionProtocol(const peer::Protocol &protocol) override;

    void addMultiplexerProtocol(const peer::Protocol &protocol) override;

    void negotiateServer(const stream::Stream &stream,
                         ChosenProtocolsCallback protocol_callback) override;

    void negotiateClient(const stream::Stream &stream,
                         ChosenProtocolsCallback protocol_callback) override;

    enum class MultiselectErrors {
      NO_PROTOCOLS_SUPPORTED = 1,
      NEGOTIATION_FAILED,
      INTERNAL_ERROR
    };

   private:
    /**
     * Read a response from the stream
     * @param stream_state - state of the stream
     */
    void readResponse(StreamState stream_state) const;

    /**
     * Process a response for our previous command
     * @param response arrived from the other side of the connection
     * @param stream_state - state of the stream
     */
    void processResponse(const kagome::common::Buffer &response,
                         StreamState stream_state) const;

    /**
     * Handle a message, signalizing about start of the negotiation
     * @param stream_state - state of the stream
     */
    void handleOpeningMsg(StreamState stream_state) const;

    /**
     * Handle a message, containing a protocol
     * @param protocol - received protocol
     * @param stream_state - state of the stream
     */
    void handleProtocolMsg(const peer::Protocol &protocol,
                           StreamState stream_state) const;

    /**
     * Handle a message, containing protocols
     * @param protocols - received protocols
     * @param stream_state - state of the stream
     */
    void handleProtocolsMsg(const std::vector<peer::Protocol> &protocols,
                            StreamState stream_state) const;

    /**
     * Handle a message, containing an ls
     * @param stream_state - state of the stream
     */
    void handleLsMsg(StreamState stream_state) const;

    /**
     * Handle a message, containing an na
     * @param stream_state - state of the stream
     */
    void handleNaMsg(StreamState stream_state) const;

    /**
     * Triggered, when a protocol msg arrives after we sent an opening or ls one
     * @param stream_state - state of the stream
     * @param protocol, which was inside the message
     */
    void onProtocolAfterOpeningOrLs(StreamState stream_state,
                                    const peer::Protocol &protocol) const;

    /**
     * Triggered, when a new message with protocols arrived, and the last
     * message we sent was an ls one
     * @param stream_state - state of the stream
     * @param received_protocols - protocols, received from the other side
     */
    void onProtocolsAfterLs(
        StreamState stream_state,
        gsl::span<const peer::Protocol> received_protocols) const;

    /**
     * Triggered, when an unexpected message arrives to as a response to our
     * request
     * @param stream_state - state of the stream
     */
    void onUnexpectedRequestResponse(StreamState stream_state) const;

    /**
     * Triggered, when a stream status contains garbage value
     * @param stream_state - state of the stream
     */
    void onGarbagedStreamStatus(StreamState stream_state) const;

    /**
     * Send a message, signalizing about start of the negotiation
     * @param stream_state - state of the stream
     */
    void sendOpeningMsg(StreamState stream_state) const;

    /**
     * Send a message, containing a protocol
     * @param protocol to be sent
     * @param stream_state - state of the stream
     */
    void sendProtocolMsg(const peer::Protocol &protocol,
                         StreamState stream_state) const;

    /**
     * Send a message, containing protocols
     * @param protocols to be sent
     * @param stream_state - state of the stream
     */
    void sendProtocolsMsg(gsl::span<const peer::Protocol> protocols,
                          StreamState stream_state) const;

    /**
     * Send a message, containing an ls
     * @param stream_state - state of the stream
     */
    void sendLsMsg(StreamState stream_state) const;

    /**
     * Send a message, containing an na
     * @param stream_state - state of the stream
     */
    void sendNaMsg(StreamState stream_state) const;

    /**
     * Triggered, when negotiation round is finished
     * @param stream_state - state of the stream
     * @param chosen_protocol - protocol, which was chosen during the round
     */
    void negotiationRoundFinished(StreamState stream_state,
                                  const peer::Protocol &chosen_protocol) const;

    /**
     * Send an ack message for the chosen protocol without waiting for a
     * response
     * @param stream for the message to be sent over
     * @param protocol - chosen protocol
     */
    void sendProtocolAck(const stream::Stream &stream,
                         const peer::Protocol &protocol) const;

    /**
     * Get a callback to be used in stream write functions
     * @param t - shared_ptr to this
     * @param stream_state - state of the stream
     * @param success_status - status to be set after a successful write
     * @param error to be shown in case of write failure
     * @return lambda-callback for the write operation
     */
    auto getWriteCallback(
        std::shared_ptr<const Multiselect> t, StreamState stream_state,
        StreamState::NegotiationStatus success_status,
        std::function<std::string(const std::error_code &ec)> error) const;

    std::vector<peer::Protocol> encryption_protocols_;
    std::vector<peer::Protocol> multiplexer_protocols_;
    kagome::common::Logger log_;
  };
}  // namespace libp2p::protocol_muxer

OUTCOME_HPP_DECLARE_ERROR(libp2p::protocol_muxer,
                          Multiselect::MultiselectErrors)

#endif  // KAGOME_MULTISELECT_IMPL_HPP
