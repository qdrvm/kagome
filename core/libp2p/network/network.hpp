/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_HPP
#define KAGOME_NETWORK_HPP

#include <outcome/outcome.hpp>

#include "libp2p/connection/capable_connection.hpp"
#include "libp2p/connection/stream.hpp"
#include "libp2p/network/router.hpp"
#include "libp2p/peer/peer_info.hpp"
#include "libp2p/peer/protocol.hpp"

namespace libp2p::network {

  // Network is the interface used to connect to the outside world.
  // It dials and listens for connections.
  // it stores all connections
  struct Network {
    virtual ~Network() = default;

    using connection_t = connection::CapableConnection;
    using connptr_t = std::shared_ptr<connection_t>;

    enum class Connectedness {
      NOT_CONNECTED = 0,
      CONNECTED,
      CAN_CONNECT,
      CAN_NOT_CONNECT
    };

    // get list of all peers connected with us
    virtual std::vector<peer::PeerId> getPeers() const = 0;

    // get list of all connections (including inbound and outbound)
    virtual std::vector<connptr_t> getConnections() const = 0;

    // get list of all outbound connections to a given peer.
    virtual std::vector<connptr_t> getConnectionsForPeer(
        const peer::PeerId &p) const = 0;

    // get best connection to a given peer
    virtual connptr_t getBestConnectionForPeer(const peer::PeerId &p) const = 0;

    // get connectedness information for given peer p
    virtual Connectedness connectedness(const peer::PeerId &p) const = 0;

    // Establishes a connection to a given peer
    virtual void dial(const peer::PeerInfo &p,
                      std::function<void(outcome::result<connptr_t>)> cb) = 0;

    // Closes all connections to a given peer
    virtual outcome::result<void> close(const peer::PeerInfo &p) = 0;

    // Listen tells the network to start listening on given multiaddr.
    // May be executed multiple times (on different addresses/protocols).
    virtual outcome::result<void> listen(const multi::Multiaddress &ma) = 0;

    // get all addresses we are listening on. may be different from those
    // supplied to `listen`. example: /ip4/0.0.0.0/tcp/0 ->
    // /ip4/127.0.0.1/tcp/30000 and /ip4/192.168.1.2/tcp/30000
    virtual std::vector<multi::Multiaddress> getListenAddresses() const = 0;

    // TODO(Warchant): emits events:
    //    ClosedStream(Network, Stream)
    //    Connected(Network, Conn)
    //    Disconnected(Network, Conn)
    //    Listen(Network, Multiaddr)
    //    ListenClose(Network, Multiaddr)
    //    OpenedStream(Network, Stream)

    // NewStream returns a new stream to given peer p.
    // If there is no connection to p, attempts to create one.
    virtual void newStream(
        const peer::PeerInfo &p, const peer::Protocol &protocol,
        const std::function<connection::Stream::Handler> &handler) = 0;

    // TODO(@warchant): check if it is needed PRE-178
    //    // SetStreamHandler sets the handler for new streams opened by the
    //    // remote side.
    //    virtual void setStreamHandler(
    //        std::function<connection::Stream::Handler> func) = 0;
    //
    //    // SetConnHandler sets the handler for new connections opened by the
    //    // remote side.
    //    virtual void setConnectionHandler(
    //        std::function<connection::Connection::Handler> func) = 0;
  };

}  // namespace libp2p::network

#endif  // KAGOME_NETWORK_HPP
