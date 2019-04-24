/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PROTOCOL_MUXER_HPP
#define KAGOME_PROTOCOL_MUXER_HPP

#include <functional>

#include <outcome/outcome.hpp>
#include "libp2p/multi/multistream.hpp"
#include "libp2p/stream/stream.hpp"

namespace libp2p::protocol_muxer {
  /**
   * Allows to negotiate with the other side of the connection about a protocol,
   * which is going to be used in communication with it
   */
  class ProtocolMuxer {
   public:
    using ChosenProtocolCallback =
        std::function<void(outcome::result<multi::Multistream>)>;

    /**
     * Add a new protocol, which can be handled by this node
     * @param protocol to be added
     */
    virtual void addProtocol(const multi::Multistream &protocol) = 0;

    /**
     * Negotiate about the protocol from the server side of the connection - we
     * are going to wait for some time for client to start negotiation; if
     * nothing happens, start it ourselves
     * @param stream to be negotiated over
     * @param protocol_callback, which is going to be called, when a protocol is
     * chosen or error occurs
     */
    virtual void negotiateServer(const stream::Stream &stream,
                                 ChosenProtocolCallback protocol_callback) = 0;

    /**
     * Negotiate about the protocol from the client side of the connection - we
     * are going to start the process immediately
     * @param stream to be negotiated over
     * @param protocol_callback, which is going to be called, when a protocol is
     * chosen or error occurs
     */
    virtual void negotiateClient(const stream::Stream &stream,
                                 ChosenProtocolCallback protocol_callback) = 0;

    virtual ~ProtocolMuxer() = default;
  };
}  // namespace libp2p::protocol_muxer

#endif  // KAGOME_PROTOCOL_MUXER_HPP
