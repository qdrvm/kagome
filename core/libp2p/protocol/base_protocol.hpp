/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BASE_PROTOCOL_HPP
#define KAGOME_BASE_PROTOCOL_HPP

#include "libp2p/connection/stream.hpp"
#include "libp2p/peer/peer_info.hpp"
#include "libp2p/peer/protocol.hpp"

namespace libp2p::protocol {

  /**
   * @brief Base class for all application-level protocols based on streams.
   */
  struct BaseProtocol {
    virtual ~BaseProtocol() = default;

    /**
     * @brief Getter for a unique identifier for this protocol.
     * @example /yamux/1.0.0 or /ping/1.0.0
     * @return protocol identifier
     */
    virtual peer::Protocol getProtocolId() const noexcept = 0;

    /**
     * @brief Server-side handler, invoked when client is opened stream to us.
     * @param stream valid stream to client
     */
    virtual void onStream(std::shared_ptr<connection::Stream> stream) = 0;

    /**
     * @brief Client-side handler, invoked when we (client) successfully
     * established connection to peer p.
     * @param p remote peer
     * @param rstream result on valid stream opened to remote peer.
     */
    virtual void newStream(
        const peer::PeerInfo &p,
        outcome::result<std::shared_ptr<connection::Stream>> rstream) = 0;
  };

}  // namespace libp2p::protocol

#endif  // KAGOME_BASE_PROTOCOL_HPP
