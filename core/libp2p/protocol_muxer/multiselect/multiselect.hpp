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

    void addProtocol(const Protocol &protocol) override;

    void negotiateServer(const stream::Stream &stream,
                         ChosenProtocolCallback protocol_callback) override;

    void negotiateClient(const stream::Stream &stream,
                         ChosenProtocolCallback protocol_callback) override;

    enum class MultiselectErrors {
      NO_PROTOCOLS_SUPPORTED = 1,
      NEGOTIATION_FAILED
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
    void handleProtocolMsg(const Protocol &protocol,
                           StreamState stream_state) const;

    /**
     * Handle a message, containing protocols
     * @param protocols - received protocols
     * @param stream_state - state of the stream
     */
    void handleProtocolsMsg(const std::vector<Protocol> &protocols,
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
     * Triggered, when a new message with protocol(s) arrived, and the last
     * message we sent was an ls one
     * @param stream_state - state of the stream
     * @param received_protocols - protocols, received from the other side
     */
    void onLsMsgSent(StreamState stream_state,
                     gsl::span<const Protocol> received_protocols) const;

    /**
     * Send a message, signalizing about start of the negotiation
     * @param stream_state - state of the stream
     */
    void sendOpeningMsg(StreamState stream_state) const;

    /**
     * Send a message, containing a protocol
     * @param protocol to be sent
     * @param wait_for_response - should the response be awaited?
     * @param stream_state - state of the stream
     */
    void sendProtocolMsg(const Protocol &protocol, bool wait_for_response,
                         StreamState stream_state) const;

    /**
     * Send a message, containing protocols
     * @param protocols to be sent
     * @param stream_state - state of the stream
     */
    void sendProtocolsMsg(gsl::span<const Protocol> protocols,
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

    std::vector<Protocol> supported_protocols_;
    kagome::common::Logger log_;
  };
}  // namespace libp2p::protocol_muxer

OUTCOME_HPP_DECLARE_ERROR(libp2p::protocol_muxer,
                          Multiselect::MultiselectErrors)

#endif  // KAGOME_MULTISELECT_IMPL_HPP
