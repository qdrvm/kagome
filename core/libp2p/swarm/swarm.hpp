/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef UGUISU_SWARM_HPP
#define UGUISU_SWARM_HPP

#include "libp2p/common_objects/peer.hpp"
#include "libp2p/connection/connection.hpp"
#include "libp2p/common_objects/multihash.hpp"
#include "libp2p/common_objects/multiaddress.hpp"

namespace libp2p {
  namespace swarm {
    /**
     * High-level interface for establishing connections with other peers
     */
    class Swarm {

      /**
       * Establish connection with the peer via the best possible transport
       * @param peer to connect to
       * @return connection
       */
      virtual std::unique_ptr<connection::Connection> dial(const common::Peer::PeerInfo &peer) = 0;

      /**
       * @see dial(PeerInfo)
       * @param peer_id - hash of peer to connect to
       * @return connection
       */
      virtual std::unique_ptr<connection::Connection> dial(const common::Multihash &peer_id) = 0;

      /**
       * @see dial(PeerInfo)
       * @param peer_address - address to connect to
       * @return connection
       */
      virtual std::unique_ptr<connection::Connection> dial(const common::Multiaddress &peer_address) = 0;

    };
  }
}

#endif //UGUISU_SWARM_HPP
