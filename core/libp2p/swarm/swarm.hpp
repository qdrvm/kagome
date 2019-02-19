/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SWARM_HPP
#define KAGOME_SWARM_HPP

#include <functional>

#include <boost/signals2.hpp>
#include "common/result.hpp"
#include "libp2p/common/peer_info.hpp"
#include "libp2p/connection/connection.hpp"
#include "libp2p/connection/connection_fsm.hpp"
#include "libp2p/error/error.hpp"
#include "libp2p/multi/multistream.hpp"
#include "libp2p/transport/transport.hpp"

namespace libp2p::swarm {
  /**
   * Manages connections with other peers
   */
  class Swarm {
   public:
    /**
     * Establish connection with the peer via the best possible transport
     * @param peer to connect to
     * @param multistream - protocol to connect over
     * @return connection in case of success, error otherwise
     */
    virtual kagome::expected::Result<connection::Connection, error::Error> dial(
        const common::PeerInfo &peer, const multi::Multistream &protocol) = 0;

    /**
     * Establish connection with the peer via the best possible transport
     * @param peer to connect to
     * @param multistream - protocol to connect over
     * @param fsm_callback - function, which is to subscribe client to
     * connection's FSM events
     * @return connection in case of success, error otherwise
     */
    virtual kagome::expected::Result<connection::Connection, error::Error> dial(
        const common::PeerInfo &peer,
        const multi::Multistream &protocol,
        std::function<void(const connection::ConnectionFSM &conn_fsm)>
            fsm_callback) = 0;

    /**
     * Hang up a connection we have with that peer
     * @param peer to stop connection with
     */
    virtual void hangUp(const common::PeerInfo &peer) = 0;

    /**
     * Handle a new protocol in this swarm
     * @param protocol to be handled
     * @param handler to be called, when a new dial is received on that protocol
     */
    virtual void handle(
        const multi::Multistream &protocol,
        std::function<void(const multi::Multistream &protocol,
                           const connection::Connection &conn)> handler) = 0;

    /**
     * Add a transport to this swarm
     * @param transport_id of that transport
     * @param transport to be added
     */
    virtual void addTransport(const std::string &transport_id,
                              transport::Transport transport) = 0;

    /**
     * Unhandle a protocol in this swarm
     * @param protocol to be unhandled
     */
    virtual void unhandle(const multi::Multistream &protocol) = 0;

    /**
     * Start listening on all added transports, reacting to received connection
     * attempts
     */
    virtual void start() = 0;

    /**
     * Close all listeners and muxers
     */
    virtual void stop() = 0;

    /**
     * Swarm is successfully started
     * @param callback to be called, when event happens
     */
    virtual void onStart(std::function<void()> callback) const = 0;

    /**
     * Swarm is stopped
     * @param callback to be called, when event happens
     */
    virtual void onStop(std::function<void()> callback) const = 0;

    /**
     * A new connection with a peer was established
     * @param callback to be called, when event happens
     */
    virtual void onNewConnection(
        std::function<void(const common::PeerInfo &peer_info)> callback)
        const = 0;

    /**
     * A connection with a peer was closed
     * @param callback to be called, when event happens
     */
    virtual void onClosedConnection(
        std::function<void(const common::PeerInfo &peer_info)> callback)
        const = 0;

    /**
     * Some error occurs in the swarm
     * @param callback to be called, when event happens
     */
    virtual void onError(
        std::function<void(const error::Error &error)> callback) const = 0;
  };
}  // namespace libp2p::swarm

#endif  // KAGOME_SWARM_HPP
