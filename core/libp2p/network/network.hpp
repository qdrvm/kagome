/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_HPP
#define KAGOME_NETWORK_HPP

#include "libp2p/network/listener.hpp"
#include "libp2p/network/dialer.hpp"

namespace libp2p::network {

  // Network is the interface used to connect to the outside world.
  struct Network {
    virtual ~Network() = default;

    // Closes all connections to a given peer
    virtual void closeConnections(const peer::PeerId &p) = 0;

    virtual Dialer& getDialer() = 0;

    virtual Listener& getListener() = 0;

    // TODO(Warchant): emits events:
    //    ClosedStream(Network, Stream)
    //    Connected(Network, Conn)
    //    Disconnected(Network, Conn)
    //    Listen(Network, Multiaddr)
    //    ListenClose(Network, Multiaddr)
    //    OpenedStream(Network, Stream)
  };

}  // namespace libp2p::network


#endif  // KAGOME_NETWORK_HPP
