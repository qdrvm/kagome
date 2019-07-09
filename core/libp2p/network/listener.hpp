/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_LISTENER_HPP
#define KAGOME_NETWORK_LISTENER_HPP

#include <vector>

#include <outcome/outcome.hpp>
#include "libp2p/multi/multiaddress.hpp"
#include "libp2p/network/router.hpp"
#include "libp2p/protocol/base_protocol.hpp"

namespace libp2p::network {

  struct Listener {
    using StreamResult = outcome::result<std::shared_ptr<connection::Stream>>;
    using StreamResultFunc = std::function<void(StreamResult)>;

    virtual ~Listener() = default;

    // returns true if network is started
    virtual bool isStarted() const = 0;

    // starts all listeners on supplied multiaddresses
    virtual void start() = 0;

    // stops listening on all multiaddresses
    virtual void stop() = 0;

    // close listener and all connections for that listener on given
    // multiaddress
    virtual outcome::result<void> closeListener(
        const multi::Multiaddress &ma) = 0;

    // Listen tells the network to start listening on given multiaddr.
    // May be executed multiple times (on different addresses/protocols).
    virtual outcome::result<void> listen(const multi::Multiaddress &ma) = 0;

    // Returns a list of addresses, added by user
    virtual std::vector<multi::Multiaddress> getListenAddresses() const = 0;

    // get all addresses we are listening on. may be different from those
    // supplied to `listen`. example: /ip4/0.0.0.0/tcp/0 ->
    // /ip4/127.0.0.1/tcp/30000 and /ip4/192.168.1.2/tcp/30000
    virtual outcome::result<std::vector<multi::Multiaddress>>
    getListenAddressesInterfaces() const = 0;

    // Add new protocol handler. Basically the same as if one would use
    // setProtocolHandler
    virtual void handleProtocol(
        std::shared_ptr<protocol::BaseProtocol> protocol) = 0;

    virtual void setProtocolHandler(const peer::Protocol &protocol,
                                    StreamResultFunc cb) = 0;

    virtual void setProtocolHandler(const peer::Protocol &protocol,
                                    StreamResultFunc cb,
                                    Router::ProtoPredicate predicate) = 0;
  };

}  // namespace libp2p::network

#endif  // KAGOME_NETWORK_LISTENER_HPP
