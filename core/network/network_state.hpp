/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_NETWORK_NETWORK_STATE_HPP
#define KAGOME_CORE_NETWORK_NETWORK_STATE_HPP

#include "network/peer_client.hpp"
#include "network/peer_server.hpp"

namespace kagome::network {

  /// Stores network's peer information
  struct NetworkState {
    std::std::shared_ptr<PeerClient> peer_client;
    std::shared_ptr<PeerServer> peer_server;
  };

}  // namespace kagome::network

#endif  // KAGOME_CORE_NETWORK_NETWORK_STATE_HPP
