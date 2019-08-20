/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_NETWORK_NETWORK_STATE_HPP
#define KAGOME_CORE_NETWORK_NETWORK_STATE_HPP

#include <unordered_map>
#include "network/peer_client.hpp"
#include "network/peer_server.hpp"

namespace libp2p::peer {
  class PeerId;
}

namespace kagome::network {

  using PeerClientsMap =
      std::unordered_map<libp2p::peer::PeerId, std::shared_ptr<PeerClient>>;

  /// Stores network's peer information
  struct NetworkState {
    PeerClientsMap peer_clients;
    std::shared_ptr<PeerServer> peer_server;
  };

}  // namespace kagome::network

#endif  // KAGOME_CORE_NETWORK_NETWORK_STATE_HPP
