/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_NETWORK_NETWORK_STATE_HPP
#define KAGOME_CORE_NETWORK_NETWORK_STATE_HPP

#include <algorithm>
#include <memory>
#include <vector>

#include <boost/assert.hpp>
#include "network/consensus_client.hpp"
#include "network/consensus_server.hpp"
#include "network/gossiper_client.hpp"
#include "network/gossiper_server.hpp"

namespace kagome::network {

  /**
   * All peers of the network, which actively participate in consensus or other
   * events
   */
  struct NetworkState {
    std::shared_ptr<ConsensusServer> consensus_server;
    std::vector<std::shared_ptr<ConsensusClient>> consensus_clients;

    std::shared_ptr<GossiperServer> gossiper_server;
    std::vector<std::shared_ptr<GossiperClient>> gossiper_clients;

    NetworkState(
        std::shared_ptr<ConsensusServer> consensus_server,
        std::vector<std::shared_ptr<ConsensusClient>> consensus_clients,
        std::shared_ptr<GossiperServer> gossiper_server,
        std::vector<std::shared_ptr<GossiperClient>> gossiper_clients)
        : consensus_server{std::move(consensus_server)},
          consensus_clients{std::move(consensus_clients)},
          gossiper_server{std::move(gossiper_server)},
          gossiper_clients{std::move(gossiper_clients)} {
      BOOST_ASSERT(consensus_server);
      BOOST_ASSERT(std::all_of(consensus_clients.begin(),
                               consensus_clients.end(),
                               [](const auto &client) { return client; }));

      BOOST_ASSERT(gossiper_server);
      BOOST_ASSERT(std::all_of(gossiper_clients.begin(),
                               gossiper_clients.end(),
                               [](const auto &client) { return client; }));
    }
  };

}  // namespace kagome::network

#endif  // KAGOME_CORE_NETWORK_NETWORK_STATE_HPP
