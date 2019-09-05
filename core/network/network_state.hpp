/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_NETWORK_NETWORK_STATE_HPP
#define KAGOME_CORE_NETWORK_NETWORK_STATE_HPP

#include <algorithm>
#include <unordered_map>

#include <boost/assert.hpp>
#include "libp2p/peer/peer_id.hpp"
#include "network/peer_client.hpp"
#include "network/peer_server.hpp"

namespace kagome::network {

  using PeerClientsMap =
      std::unordered_map<libp2p::peer::PeerId, std::shared_ptr<PeerClient>>;

  /// Stores network's peer information
  struct NetworkState {
    PeerClientsMap peer_clients;
    std::shared_ptr<PeerServer> peer_server;

    NetworkState(PeerClientsMap _peer_clients,
                 std::shared_ptr<PeerServer> _peer_server)
        : peer_clients{std::move(_peer_clients)},
          peer_server{std::move(_peer_server)} {
      BOOST_ASSERT(std::all_of(peer_clients.begin(),
                               peer_clients.end(),
                               [](const auto &pair) { return pair.second; }));
      BOOST_ASSERT(peer_server);
    }
  };

}  // namespace kagome::network

#endif  // KAGOME_CORE_NETWORK_NETWORK_STATE_HPP
