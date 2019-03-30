/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TRANSPORT_HPP
#define KAGOME_TRANSPORT_HPP

#include <chrono>
#include <functional>
#include <memory>
#include <outcome/outcome.hpp>  // for outcome::result

#include "libp2p/multi/multiaddress.hpp"
#include "libp2p/transport/connection.hpp"
#include "libp2p/transport/transport_listener.hpp"

namespace libp2p::transport {
  /**
   * Allows to establish connections with other peers and react to received
   * attempts to do so; can be implemented, for example, as TCP, UDP etc
   */
  class Transport : public basic::Closeable {
   public:
    virtual ~Transport() = default;

    /**
     * Try to establish connection with a peer
     * @param address of the peer
     * @return connection in case of success, error otherwise
     */
    virtual outcome::result<std::shared_ptr<Connection>> dial(
        const multi::Multiaddress &address) = 0;

    /**
     * Create a listener for incoming connections of this Transport; in case
     * it was already created, return it
     * @return pointer to the created listener
     */
    virtual std::shared_ptr<TransportListener> createListener(
        TransportListener::HandlerFunc handler) = 0;
  };
}  // namespace libp2p::transport

#endif  // KAGOME_TRANSPORT_HPP
