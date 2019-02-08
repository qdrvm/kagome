/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef UGUISU_PEER_DISCOVERY_HPP
#define UGUISU_PEER_DISCOVERY_HPP

#include <boost/signals2.hpp>
#include "libp2p/common_objects/peer.hpp"

namespace libp2p {
  namespace discovery {
    class PeerDiscovery {
      /**
       * Return observable to peers, discovered by this module
       * @return observable to found peers
       */
      virtual boost::signals2::signal<void(common::Peer::PeerInfo)>
      getNewPeers() const = 0;
    };
  }  // namespace discovery
}  // namespace libp2p

#endif  // UGUISU_PEER_DISCOVERY_HPP
