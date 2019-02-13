/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SWARM_HPP
#define KAGOME_SWARM_HPP

#include <boost/signals2.hpp>
#include <rxcpp/rx-observable.hpp>
#include "libp2p/common_objects/peer_info.hpp"
#include "libp2p/connection/connection.hpp"
#include "libp2p/connection/connection_status.hpp"

namespace libp2p {
  namespace swarm {
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
       * Emit, when swarm is successfully started
       * @return signal to this event
       */
      virtual boost::signals2::signal<void()> onStart() const = 0;

      /**
       * Emit, when swarm is stopped
       * @return signal to this event
       */
      virtual boost::signals2::signal<void()> onStop() const = 0;

      /**
       * Emit, when a new connection with peer was established
       * @return signal to this event
       */
      virtual boost::signals2::signal<void(common::PeerInfo)> onNewConnection()
          const = 0;

      /**
       * Emit, when connection with peer was closed
       * @return signal to this event
       */
      virtual boost::signals2::signal<void(common::PeerInfo)>
      onClosedConnection() const = 0;

      /**
       * Emit, when some error occurs in the swarm
       * @return signal to this event
       */
      virtual boost::signals2::signal<void(std::string)> onError() const = 0;
    };
  }  // namespace swarm
}  // namespace libp2p

#endif  // KAGOME_SWARM_HPP
