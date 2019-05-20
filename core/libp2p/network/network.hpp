/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_HPP
#define KAGOME_NETWORK_HPP

#include <outcome/outcome.hpp>

#include "libp2p/peer/peer_info.hpp"
#include "libp2p/peer/protocol.hpp"
#include "libp2p/stream/stream.hpp"
#include "libp2p/swarm/connection_manager.hpp"
#include "libp2p/swarm/router.hpp"

namespace libp2p::swarm {

  // Network is the interface used to connect to the outside world.
  // It dials and listens for connections.
  struct Network {
    virtual ~Network() = default;

    virtual ConnectionManager &getConnectionManager() = 0;

    // NewStream returns a new stream to given peer p.
    // If there is no connection to p, attempts to create one.
    virtual outcome::result<void> newStream(
        const peer::PeerInfo &p, const peer::Protocol &protocol,
        const std::function<stream::Stream::Handler> &handler) = 0;

    // SetStreamHandler sets the handler for new streams opened by the
    // remote side.
    virtual void setStreamHandler(
        std::function<stream::Stream::Handler> func) = 0;

    // SetConnHandler sets the handler for new connections opened by the
    // remote side.
    virtual void setConnectionHandler(
        std::function<transport::Connection::Handler> func) = 0;
  };

}  // namespace libp2p::swarm

#endif  // KAGOME_NETWORK_HPP
