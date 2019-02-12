/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PEER_DISCOVERY_HPP
#define KAGOME_PEER_DISCOVERY_HPP

#include <rxcpp/rx-observable.hpp>
#include "libp2p/common_objects/peer.hpp"

namespace libp2p {
  namespace discovery {
    class PeerDiscovery {
      /**
       * Return observable to peers, discovered by this module
       * @return observable to new peers
       */
      virtual rxcpp::observable<common::Peer::PeerInfo> getNewPeers() const = 0;
    };
  }  // namespace discovery
}  // namespace libp2p

#endif  // KAGOME_PEER_DISCOVERY_HPP
