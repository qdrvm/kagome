/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MULTISELECT_HPP
#define KAGOME_MULTISELECT_HPP

#include <memory>
#include <string>
#include <vector>

#include <gsl/span>
#include "common/logger.hpp"
#include "libp2p/peer/peer_id.hpp"
#include "libp2p/protocol_muxer/multiselect/message_manager.hpp"
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
    using PeerId = multi::Multihash;

    // TODO(akvinikym) [PRE-127] 25.04.19: think about passing not a PeerId, but
    // an Identity service, when it's implemented
    // Also accept PeerIdConverter in order to base58 it
    /**
     * Create a Multiselect instance
     * @param peer_id - id of the current peer
     * @param logger to write debug messages to
     */
    explicit Multiselect(std::shared_ptr<peer::PeerId> peer_id,
                         kagome::common::Logger logger =
                             kagome::common::createLogger("Multiselect"));

    ~Multiselect() override;

    void addProtocol(const multi::Multistream &protocol) override;

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
    void handleProtocolMsg(const multi::Multistream &protocol,
                           StreamState stream_state) const;

    /**
     * Handle a message, containing protocols
     * @param protocols - received protocols
     * @param stream_state - state of the stream
     */
    void handleProtocolsMsg(const std::vector<multi::Multistream> &protocols,
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
    void sendProtocolMsg(const multi::Multistream &protocol,
                         bool wait_for_response,
                         StreamState stream_state) const;

    /**
     * Send a message, containing protocols
     * @param protocols to be sent
     * @param stream_state - state of the stream
     */
    void sendProtocolsMsg(gsl::span<const multi::Multistream> protocols,
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

    MessageManager message_manager_;
    std::vector<multi::Multistream> supported_protocols_;
    std::shared_ptr<peer::PeerId> peer_id_;

    kagome::common::Logger log_;
  };
}  // namespace libp2p::protocol_muxer

OUTCOME_HPP_DECLARE_ERROR(libp2p::protocol_muxer,
                          Multiselect::MultiselectErrors)

#endif  // KAGOME_MULTISELECT_HPP
