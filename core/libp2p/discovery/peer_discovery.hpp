/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PEER_DISCOVERY_HPP
#define KAGOME_PEER_DISCOVERY_HPP

#include <vector>

#include "libp2p/common/peer_info.hpp"

namespace libp2p::discovery {
  /**
   * Continuously scans the network to find and cache new peers
   */
  class PeerDiscovery {
    /**
     * Return observable to peers, discovered by this module
     * @return observable to new peers
     */
    // TODO(@warchant): PRE-90 review types, rewrite docs
    virtual std::vector<common::PeerInfo> getNewPeers() const = 0;

    /**
     * Return all peers, which were found by this node
     * @return reference to PeerInfo collection
     */
    virtual const std::vector<common::PeerInfo> &getAllPeers() const = 0;
  };
}  // namespace libp2p::discovery

#endif  // KAGOME_PEER_DISCOVERY_HPP
