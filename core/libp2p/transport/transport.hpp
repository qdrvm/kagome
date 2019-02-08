/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef UGUISU_TRANSPORT_HPP
#define UGUISU_TRANSPORT_HPP

#include <functional>
#include <memory>

#include "libp2p/common_objects/multiaddress.hpp"
#include "libp2p/connection/connection.hpp"
#include "libp2p/transport/transport_listener.hpp"

namespace libp2p {
  namespace transport {
    /**
     * Allows to establish connections with other peers and react to received
     * attempts to do so; can be implemented, for example, as TCP, UPD etc
     */
    class Transport {
     public:
      /**
       * Try to establish connection with a peer
       * @param address of the peer
       * @return pointer to connection to that peer
       */
      virtual std::unique_ptr<connection::Connection> dial(
          const common::Multiaddress &address) = 0;

      /**
       * Create a listener for incoming connections of this Transport
       * @param handler, which is going to be invoked every time a new
       * connection received
       * @return pointer to the created listener
       */
      virtual std::unique_ptr<TransportListener> createListener(
          std::function<void()> handler) = 0;
    };
  }  // namespace transport
}  // namespace libp2p

#endif  // UGUISU_TRANSPORT_HPP
