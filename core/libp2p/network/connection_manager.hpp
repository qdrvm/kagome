/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONNECTION_MANAGER_HPP
#define KAGOME_CONNECTION_MANAGER_HPP

#include <memory>

#include "libp2p/basic/garbage_collectable.hpp"
#include "libp2p/connection/capable_connection.hpp"

namespace libp2p::network {

  // TODO(warchant): when connection is closed ('onDisconnected' event fired),
  // manager should remove it from storage PRE-212

  struct ConnectionManager : public basic::GarbageCollectable {
    using Connection = connection::CapableConnection;
    using ConnectionSPtr = std::shared_ptr<Connection>;

    enum class Connectedness {
      NOT_CONNECTED,  ///< we don't know peer's addresses, and are not connected
      CONNECTED,      ///< we have at least one connection to this peer
      CAN_CONNECT,  ///< we know address of this peer, and we can dial to create
                    ///< a connection
      CAN_NOT_CONNECT  ///< we know address of this peer, but can not dial (no
      ///< transports supported)
    };

    ~ConnectionManager() override = default;

    // get list of all connections (including inbound and outbound)
    virtual std::vector<ConnectionSPtr> getConnections() const = 0;

    // get list of all outbound connections to a given peer.
    virtual std::vector<ConnectionSPtr> getConnectionsToPeer(
        const peer::PeerId &p) const = 0;

    // get best connection to a given peer
    virtual ConnectionSPtr getBestConnectionForPeer(
        const peer::PeerId &p) const = 0;

    // get connectedness information for given peer p
    virtual Connectedness connectedness(const peer::PeerId &p) const = 0;

    // add connection to a given peer
    virtual void addConnectionToPeer(const peer::PeerId &p,
                                     ConnectionSPtr c) = 0;
  };

}  // namespace libp2p::network

#endif  // KAGOME_CONNECTION_MANAGER_HPP
