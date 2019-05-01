/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PROTOCOL_MUXER_HPP
#define KAGOME_PROTOCOL_MUXER_HPP

#include <functional>

#include <outcome/outcome.hpp>
#include "libp2p/peer/protocol.hpp"
#include "libp2p/stream/stream.hpp"

namespace libp2p::protocol_muxer {
  /**
   * Allows to negotiate with the other side of the connection about the
   * protocols, which are going to be used in communication with it
   */
  class ProtocolMuxer {
   public:
    /**
     * Pair of protocols, on which the two sides negotiated
     */
    struct NegotiatedProtocols {
      peer::Protocol encryption_protocol_;
      peer::Protocol multiplexer_protocol_;
    };
    using ChosenProtocolsCallback =
        std::function<void(outcome::result<NegotiatedProtocols>)>;

    /**
     * Add a new encryption protocol, which can be handled by this node
     * @param protocol to be added
     */
    virtual void addEncryptionProtocol(const peer::Protocol &protocol) = 0;

    /**
     * Add a new multiplexer protocol, which can be handled by this node
     * @param protocol to be added
     */
    virtual void addMultiplexerProtocol(const peer::Protocol &protocol) = 0;

    /**
     * Negotiate about the protocols from the server side of the connection - we
     * are going to wait for some time for client to start negotiation; if
     * nothing happens, start it ourselves
     * @param stream to be negotiated over
     * @param protocols_callback, which is going to be called, when the
     * protocols are chosen or error occurs
     */
    virtual void negotiateServer(
        const stream::Stream &stream,
        ChosenProtocolsCallback protocols_callback) = 0;

    /**
     * Negotiate about the protocols from the client side of the connection - we
     * are going to start the process immediately
     * @param stream to be negotiated over
     * @param protocols_callback, which is going to be called, when the
     * protocols are chosen or error occurs
     */
    virtual void negotiateClient(
        const stream::Stream &stream,
        ChosenProtocolsCallback protocols_callback) = 0;

    virtual ~ProtocolMuxer() = default;
  };
}  // namespace libp2p::protocol_muxer

#endif  // KAGOME_PROTOCOL_MUXER_HPP
