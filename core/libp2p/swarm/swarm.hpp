/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SWARM_HPP
#define KAGOME_SWARM_HPP

#include <functional>

#include <boost/signals2.hpp>
#include <rxcpp/rx-observable.hpp>
#include "libp2p/common/peer_info.hpp"
#include "libp2p/connection/connection.hpp"
#include "libp2p/connection/connection_status.hpp"

namespace libp2p::swarm {
  /**
   * Establishes connections with other peers
   */
  class Swarm {
   public:
    /**
     * Establish connection with the peer via the best possible transport
     * @param peer to connect to
     * @return observable to connection's statuses and connection itself in
     * case of success
     */
    virtual rxcpp::observable<connection::ConnectionStatus> dial(
        const common::PeerInfo &peer) = 0;

    /**
     * Hang up a connection we have with that peer
     * @param peer to stop connection with
     */
    virtual void hangUp(const common::PeerInfo &peer) = 0;

    /**
     * Start listening on all added transports
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
        std::function<void(common::PeerInfo)> callback) const = 0;

    /**
     * A connection with a peer was closed
     * @param callback to be called, when event happens
     */
    virtual void onClosedConnection(
        std::function<void(common::PeerInfo)> callback) const = 0;

    /**
     * Some error occurs in the swarm
     * @param callback to be called, when event happens
     */
    virtual void onError(std::function<void(std::string)> callback) const = 0;
  };
}  // namespace libp2p::swarm

#endif  // KAGOME_SWARM_HPP
