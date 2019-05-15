/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONNECTION_MANAGER_HPP
#define KAGOME_CONNECTION_MANAGER_HPP

#include <vector>

#include "libp2p/multi/multiaddress.hpp"
#include "libp2p/peer/peer_id.hpp"
#include "libp2p/peer/peer_info.hpp"
#include "libp2p/transport/connection.hpp"

namespace libp2p::swarm {

  // connections here are already muxed and secured, and ready-to-be-used

  struct ConnectionManager {
    using sptrconn = std::shared_ptr<transport::Connection>;

    virtual ~ConnectionManager() = default;

    virtual std::vector<sptrconn> getConnections() const = 0;

    virtual std::vector<sptrconn> getConnectionsForPeer(
        const peer::PeerId &p) const = 0;

    virtual outcome::result<sptrconn> dial(const peer::PeerInfo &p) = 0;

    virtual outcome::result<void> listen(const multi::Multiaddress &ma) = 0;

    // get all addresses we are listening on. may be different from those
    // supplied to `listen`. example: /ip4/0.0.0.0/tcp/0 ->
    // /ip4/127.0.0.1/tcp/30000 and /ip4/192.168.1.2/tcp/30000
    virtual std::vector<multi::Multiaddress> getListenAddresses() const = 0;
  };

}  // namespace libp2p::swarm

#endif  // KAGOME_CONNECTION_MANAGER_HPP
