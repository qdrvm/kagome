/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PROTOCOL_MUXER_HPP
#define KAGOME_PROTOCOL_MUXER_HPP

#include <functional>
#include <memory>

#include <outcome/outcome.hpp>
#include "libp2p/peer/protocol.hpp"
#include "libp2p/stream/stream.hpp"
#include "libp2p/transport/connection.hpp"

namespace libp2p::protocol_muxer {
  /**
   * Allows to negotiate with the other side of the connection about the
   * protocols, which are going to be used in communication with it
   */
  class ProtocolMuxer {
   public:
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
     * Add a new stream protocol, which can be handled by this node
     * @param protocol to be added
     */
    virtual void addStreamProtocol(const peer::Protocol &protocol) = 0;

    using ChosenProtocolCallback =
        std::function<void(outcome::result<peer::Protocol>)>;

    /**
     * Negotiate about the encryption protocol with the other side
     * @param connection to be negotiated over
     * @param protocol_callback, which is going to be called, when the
     * protocol is chosen or error occurs
     */
    virtual void negotiateEncryption(
        std::shared_ptr<transport::Connection> connection,
        ChosenProtocolCallback protocol_callback) = 0;

    /**
     * Negotiate about the multiplexer protocol with the other side
     * @param connection to be negotiated over
     * @param protocol_callback, which is going to be called, when the
     * protocol is chosen or error occurs
     */
    virtual void negotiateMultiplexer(
        std::shared_ptr<transport::Connection> connection,
        ChosenProtocolCallback protocol_callback) = 0;

    using ChosenProtocolAndStreamCallback = std::function<void(
        outcome::result<peer::Protocol>, std::unique_ptr<stream::Stream>)>;

    /**
     * Negotiate about the stream protocol with the other side
     * @param stream to be negotiated over
     * @param cb, which is going to be called, when the
     * protocol is chosen or error occurs
     */
    virtual void negotiateStream(std::unique_ptr<stream::Stream> stream,
                                 ChosenProtocolAndStreamCallback cb) = 0;

    virtual ~ProtocolMuxer() = default;
  };
}  // namespace libp2p::protocol_muxer

#endif  // KAGOME_PROTOCOL_MUXER_HPP
