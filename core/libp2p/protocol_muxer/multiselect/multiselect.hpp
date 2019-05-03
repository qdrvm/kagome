/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MULTISELECT_IMPL_HPP
#define KAGOME_MULTISELECT_IMPL_HPP

#include <memory>
#include <queue>
#include <string>
#include <string_view>
#include <vector>

#include <boost/asio/streambuf.hpp>
#include <gsl/span>
#include "common/logger.hpp"
#include "libp2p/protocol_muxer/multiselect/connection_state.hpp"
#include "libp2p/protocol_muxer/multiselect/message_manager.hpp"
#include "libp2p/protocol_muxer/multiselect/message_reader.hpp"
#include "libp2p/protocol_muxer/multiselect/message_writer.hpp"
#include "libp2p/protocol_muxer/protocol_muxer.hpp"

namespace libp2p::protocol_muxer {
  /**
   * Implementation of a protocol muxer. Read more
   * https://github.com/multiformats/multistream-select
   */
  class Multiselect : public ProtocolMuxer,
                      public std::enable_shared_from_this<Multiselect> {
    friend MessageWriter;
    friend MessageReader;

   public:
    /**
     * Create a Multiselect instance
     * @param logger to write debug messages to
     */
    explicit Multiselect(kagome::common::Logger logger =
                             kagome::common::createLogger("Multiselect"));

    Multiselect(const Multiselect &other) = delete;
    Multiselect &operator=(const Multiselect &other) = delete;
    Multiselect(Multiselect &&other) = default;
    Multiselect &operator=(Multiselect &&other) = default;

    ~Multiselect() override = default;

    void addEncryptionProtocol(const peer::Protocol &protocol) override;

    void addMultiplexerProtocol(const peer::Protocol &protocol) override;

    void addStreamProtocol(const peer::Protocol &protocol) override;

    void negotiateServer(std::shared_ptr<transport::Connection> connection,
                         ChosenProtocolsCallback protocol_callback) override;

    void negotiateClient(std::shared_ptr<transport::Connection> connection,
                         ChosenProtocolsCallback protocol_callback) override;

    enum class MultiselectErrors {
      NO_PROTOCOLS_SUPPORTED = 1,
      NEGOTIATION_FAILED,
      INTERNAL_ERROR
    };

   private:
    void onWriteCompleted(ConnectionState connection_state) const;

    void onWriteAckCompleted(ConnectionState connection_state,
                             const peer::Protocol &protocol);

    void onReadCompleted(ConnectionState connection_state,
                         MessageManager::MultiselectMessage msg);

    void onError(ConnectionState connection_state, std::string_view error);

    void onError(ConnectionState connection_state, std::string_view error,
                 const std::error_code &ec);

    /**
     * Process a response for our previous command
     * @param connection_state - state of the connection
     * @param msg arrived from the other side of the connection
     */
    void processResponse(ConnectionState connection_state,
                         MessageManager::MultiselectMessage msg);

    /**
     * Handle a message, signalizing about start of the negotiation
     * @param connection_state - state of the connection
     */
    void handleOpeningMsg(ConnectionState connection_state) const;

    /**
     * Handle a message, containing a protocol
     * @param protocol - received protocol
     * @param connection_state - state of the connection
     */
    void handleProtocolMsg(const peer::Protocol &protocol,
                           ConnectionState connection_state);

    /**
     * Handle a message, containing protocols
     * @param protocols - received protocols
     * @param connection_state - state of the connection
     */
    void handleProtocolsMsg(const std::vector<peer::Protocol> &protocols,
                            ConnectionState connection_state) const;

    /**
     * Handle a message, containing an ls
     * @param connection_state - state of the connection
     */
    void handleLsMsg(ConnectionState connection_state) const;

    /**
     * Handle a message, containing an na
     * @param connection_state - state of the connection
     */
    void handleNaMsg(ConnectionState connection_state) const;

    /**
     * Triggered, when a protocol msg arrives after we sent an opening or ls one
     * @param connection_state - state of the connection
     * @param protocol, which was inside the message
     */
    void onProtocolAfterOpeningOrLs(ConnectionState connection_state,
                                    const peer::Protocol &protocol) const;

    /**
     * Triggered, when a new message with protocols arrived, and the last
     * message we sent was an ls one
     * @param connection_state - state of the connection
     * @param received_protocols - protocols, received from the other side
     */
    void onProtocolsAfterLs(
        ConnectionState connection_state,
        gsl::span<const peer::Protocol> received_protocols) const;

    /**
     * Triggered, when an unexpected message arrives to as a response to our
     * request
     * @param connection_state - state of the connection
     */
    void onUnexpectedRequestResponse(ConnectionState connection_state) const;

    /**
     * Triggered, when a stream status contains garbage value
     * @param connection_state - state of the connection
     */
    void onGarbagedStreamStatus(ConnectionState connection_state) const;

    /**
     * Triggered, when negotiation round is finished
     * @param connection_state - state of the connection
     * @param chosen_protocol - protocol, which was chosen during the round
     */
    void negotiationRoundFinished(ConnectionState connection_state,
                                  const peer::Protocol &chosen_protocol);

    /**
     * Get a collection of protocols, which are available for a particular round
     * @param round, for which the protocols are to be retrieved
     * @return the protocols
     */
    gsl::span<const peer::Protocol> getProtocolsByRound(
        ConnectionState::NegotiationRound round) const;

    /**
     * Clear the resources, which left after the provided connection state
     * @param connection_state - state of the connection
     */
    void clearResources(const ConnectionState &connection_state);

    std::vector<peer::Protocol> encryption_protocols_;
    std::vector<peer::Protocol> multiplexer_protocols_;
    std::vector<peer::Protocol> stream_protocols_;
    kagome::common::Logger log_;

    std::vector<std::shared_ptr<kagome::common::Buffer>> write_buffers_;
    std::vector<std::shared_ptr<boost::asio::streambuf>> read_buffers_;
    std::queue<size_t> free_buffers_;
  };
}  // namespace libp2p::protocol_muxer

OUTCOME_HPP_DECLARE_ERROR(libp2p::protocol_muxer,
                          Multiselect::MultiselectErrors)

#endif  // KAGOME_MULTISELECT_IMPL_HPP
