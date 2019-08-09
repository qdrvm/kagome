/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_NETWORK_NETWORK_HPP
#define KAGOME_CORE_CONSENSUS_NETWORK_NETWORK_HPP

#include "common/buffer.hpp"
#include "libp2p/connection/stream.hpp"
#include "libp2p/peer/peer_info.hpp"
#include "libp2p/peer/protocol.hpp"

namespace kagome::consensus {

  /**
   * Network abstraction for consensus networking
   */
  class Network {
   public:
    /**
     * Add \param handler for \param protocol
     */
    virtual void setProtocolHandler(
        const libp2p::peer::Protocol &protocol,
        const std::function<libp2p::connection::Stream::Handler> &handler,
        const std::function<bool(const libp2p::peer::Protocol &)>
            &predicate) = 0;

    /**
     * Send \param message over a \param protocol to \param peer
     */
    virtual void sendMessage(const libp2p::peer::PeerInfo peer,
                             const libp2p::peer::Protocol &protocol,
                             const common::Buffer &message);

    /**
     * Create connection for peer \param p
     */
    virtual void connect(const libp2p::peer::PeerInfo &p) = 0;

    /**
     * Get list of all connected peers
     */
    virtual const std::vector<libp2p::peer::PeerInfo> &getPeers() const;
  };

}  // namespace kagome::consensus

#endif  // KAGOME_CORE_CONSENSUS_NETWORK_NETWORK_HPP
