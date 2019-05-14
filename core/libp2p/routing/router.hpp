/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_ROUTER_HPP
#define KAGOME_ROUTER_HPP

#include <queue>

//#include "libp2p/common/peer_info.hpp"
#include "libp2p/multi/multihash.hpp"

namespace libp2p::routing {
  /**
   * Provides a way to find other peers in the network to establish
   * connections with them
   */
  struct PeerRouting {
    virtual ~PeerRouting() = default;
    //    /**
    //     * Find peers 'responsible' or 'closest' to a given key; explanation
    //     of
    //     * these terms can be found in the articles about Kademlia DHT
    //     * @param key to be searched for
    //     * @return collection of found peers, sorted by 'closeness' to the key
    //     */
    //    virtual std::priority_queue<common::PeerInfo> findPeers(
    //        const multi::Multihash &key) const = 0;
    //
    //    /**
    //     * Save a peer to the storage to be able to find it later
    //     * @param peer to be saved
    //     */
    //    virtual void submitPeer(const common::PeerInfo &peer) = 0;
  };
}  // namespace libp2p::routing

#endif  // KAGOME_ROUTER_HPP
