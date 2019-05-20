/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONNECTION_MANAGER_HPP
#define KAGOME_CONNECTION_MANAGER_HPP

#include <gsl/span>
#include "libp2p/multi/multiaddress.hpp"
#include "libp2p/peer/peer_id.hpp"
#include "libp2p/peer/peer_info.hpp"
#include "libp2p/transport/connection.hpp"

namespace libp2p::network {

  // connections here are already muxed and secured, and ready-to-be-used
  struct ConnectionManager {
    using sptrconn = std::shared_ptr<transport::Connection>;

    virtual ~ConnectionManager() = default;

    enum class Connectedness {
      NOT_CONNECTED = 0,
      CONNECTED,
      CAN_CONNECT,
      CAN_NOT_CONNECT
    };

    // get list of all peers connected with us
    virtual std::vector<peer::PeerId> getPeers() const = 0;

    // get list of all connections (including inbound and outbound)
    virtual std::vector<sptrconn> getConnections() const = 0;

    // get list of all outbound connections to a given peer.
    virtual std::vector<sptrconn> getConnectionsForPeer(
        const peer::PeerId &p) const = 0;

    // get best connection to a given peer
    virtual sptrconn getBestConnectionForPeer(const peer::PeerId &p) const = 0;

    // get connectedness information for given peer p
    virtual Connectedness connectedness(const peer::PeerId &p) const = 0;

    // Establishes a connection to a given peer
    virtual outcome::result<sptrconn> dial(const peer::PeerInfo &p) = 0;

    // Closes all connections to a given peer
    virtual outcome::result<void> close(const peer::PeerInfo &p) = 0;

    // Listen tells the network to start listening on given multiaddr.
    // May be executed multiple times (on different addresses/protocols).
    virtual outcome::result<void> listen(const multi::Multiaddress &ma) = 0;

    // get all addresses we are listening on. may be different from those
    // supplied to `listen`. example: /ip4/0.0.0.0/tcp/0 ->
    // /ip4/127.0.0.1/tcp/30000 and /ip4/192.168.1.2/tcp/30000
    virtual gsl::span<const multi::Multiaddress> getListenAddresses() const = 0;

    // TODO: emits events:
    //    ClosedStream(Network, Stream)
    //    Connected(Network, Conn)
    //    Disconnected(Network, Conn)
    //    Listen(Network, Multiaddr)
    //    ListenClose(Network, Multiaddr)
    //    OpenedStream(Network, Stream)
  };

}  // namespace libp2p::network

#endif  // KAGOME_CONNECTION_MANAGER_HPP
