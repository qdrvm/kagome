/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_ROUTER_HPP
#define KAGOME_ROUTER_HPP

#include <queue>

#include "libp2p/common_objects/multihash.hpp"
#include "libp2p/common_objects/peer.hpp"

namespace libp2p {
  namespace routing {
    /**
     * Provides a way to find other peers in the network to establish
     * connections with them
     */
    class Router {
     public:
      /**
       * Find peers 'responsible' or 'closest' to a given key; explanation of
       * these terms can be found in the articles about Kademlia DHT
       * @param key to be searched for
       * @return collection of found peers, sorted by 'closeness' to the key
       */
      virtual std::priority_queue<common::Peer::PeerInfo> findPeers(
          const common::Multihash &key) const = 0;

      /**
       * Save a peer to the storage to be able to find it later
       * @param peer to be saved
       */
      virtual void submitPeer(const common::Peer::PeerInfo &peer) = 0;
    };
  }  // namespace routing
}  // namespace libp2p

#endif  // KAGOME_ROUTER_HPP
