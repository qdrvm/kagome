/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TRANSPORT_HPP
#define KAGOME_TRANSPORT_HPP

#include <chrono>
#include <functional>
#include <memory>
#include <system_error>

#include <outcome/outcome.hpp>  // for outcome::result
#include "libp2p/connection/capable_connection.hpp"
#include "libp2p/event/emitter.hpp"
#include "libp2p/multi/multiaddress.hpp"
#include "libp2p/peer/peer_id.hpp"
#include "libp2p/transport/transport_listener.hpp"

namespace libp2p::transport {

  /**
   * Allows to establish connections with other peers and react to received
   * attempts to do so; can be implemented, for example, as TCP, UDP etc
   */
  class Transport {
   public:
    using ConnectionCallback =
        void(outcome::result<std::shared_ptr<connection::CapableConnection>>);
    using HandlerFunc = std::function<ConnectionCallback>;

    virtual ~Transport() = default;

    /**
     * Try to establish connection with a peer
     * @param address of the peer
     * @param handler callback that will be executed on connection/error
     * @return connection in case of success, error otherwise
     */
    virtual void dial(const multi::Multiaddress &address,
                      HandlerFunc handler) const = 0;

    /**
     * Create a listener for incoming connections of this Transport; in case
     * it was already created, return it
     * @param handler callback that will be executed on new connection
     * @return pointer to the created listener
     */
    virtual std::shared_ptr<TransportListener> createListener(
        TransportListener::HandlerFunc handler) const = 0;

    /**
     * Check if this transport supports a given multiaddress
     * @param ma to be checked against
     * @return true, if transport supports that multiaddress, false otherwise
     * @note example: '/tcp/...' on tcp transport will return true
     */
    virtual bool canDial(const multi::Multiaddress &ma) const = 0;
  };
}  // namespace libp2p::transport

#endif  // KAGOME_TRANSPORT_HPP
