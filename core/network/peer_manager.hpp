/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_PEERMANAGER
#define KAGOME_NETWORK_PEERMANAGER

#include <libp2p/peer/peer_id.hpp>
#include <libp2p/peer/peer_info.hpp>

namespace kagome::network {
  /**
   *
   */
  class PeerManager {
   public:
    using PeerId = libp2p::peer::PeerId;
    using PeerInfo = libp2p::peer::PeerInfo;

    virtual ~PeerManager() = default;

    virtual void connectToPeer(const PeerInfo &peer_info) = 0;

    virtual void forEachPeer(
        std::function<void(const PeerId &)> func) const = 0;
    virtual void forOnePeer(const PeerId &peer_id,
                            std::function<void()> func) const = 0;
  };
}  // namespace kagome::network

#endif  // KAGOME_NETWORK_PEERMANAGER
