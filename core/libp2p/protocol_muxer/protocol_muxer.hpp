/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PROTOCOL_MUXER_HPP
#define KAGOME_PROTOCOL_MUXER_HPP

#include <functional>
#include <memory>

#include <outcome/outcome.hpp>
#include "libp2p/connection/raw_connection.hpp"
#include "libp2p/connection/secure_connection.hpp"
#include "libp2p/connection/stream.hpp"
#include "libp2p/peer/protocol.hpp"

namespace libp2p::protocol_muxer {
  /**
   * Allows to negotiate with the other side of the connection about the
   * protocols, which are going to be used in communication with it
   */
  class ProtocolMuxer {
   public:
    // TODO(akvinikym) 28.05.19 [PRE-189]: adders below are to be removed, when
    // ProtocolRepository is reworked
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

    /**
     * Negotiate about the encryption protocol with the other side
     * @param connection to be negotiated over
     * @return chosen protocol or error
     */
    virtual outcome::result<peer::Protocol> negotiateEncryption(
        std::shared_ptr<connection::RawConnection> connection) = 0;

    /**
     * Negotiate about the multiplexer protocol with the other side
     * @param connection to be negotiated over
     * @return chosen protocol or error
     */
    virtual outcome::result<peer::Protocol> negotiateMultiplexer(
        std::shared_ptr<connection::SecureConnection> connection) = 0;

    /**
     * Negotiate about the stream protocol with the other side
     * @param stream to be negotiated over
     * @return chosen protocol or error
     */
    virtual outcome::result<peer::Protocol> negotiateAppProtocol(
        std::shared_ptr<connection::Stream> stream) = 0;

    virtual ~ProtocolMuxer() = default;
  };
}  // namespace libp2p::protocol_muxer

#endif  // KAGOME_PROTOCOL_MUXER_HPP
