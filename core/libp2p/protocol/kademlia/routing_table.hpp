/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_KAD_ROUTING_TABLE_HPP
#define KAGOME_KAD_ROUTING_TABLE_HPP

#include "libp2p/protocol/kademlia/common.hpp"

namespace libp2p::protocol::kademlia {

  struct RoutingTable {
    virtual ~RoutingTable() = default;

    using PeerIdVecResult = outcome::result<PeerIdVec>;
    using PeerIdVecResultFunc = std::function<void(PeerIdVecResult)>;

    /// Update adds or moves the given peer to the front of its respective
    /// bucket
    virtual outcome::result<peer::PeerId> update(const peer::PeerId &pid) = 0;

    /// Remove deletes a peer from the routing table. This is to be used
    /// when we are sure a node has disconnected completely.
    virtual void remove(const NodeId &id) = 0;

    virtual PeerIdVec getAllPeers() const = 0;

    /// NearestPeers returns a list of the 'count' closest peers to the given ID
    virtual void getNearestPeers(const NodeId &id, size_t count, PeerIdVecResultFunc f) = 0;

    /// Size returns the total number of peers in the routing table
    virtual size_t size() const = 0;
  };

}  // namespace libp2p::protocol::kademlia

#endif  // KAGOME_KAD_ROUTING_TABLE_HPP
