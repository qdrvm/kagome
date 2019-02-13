/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TRANSPORT_LISTENER_HPP
#define KAGOME_TRANSPORT_LISTENER_HPP

#include <vector>

#include <boost/optional.hpp>
#include <boost/signals2.hpp>
#include "libp2p/connection/connection.hpp"
#include "libp2p/multi/multiaddress.hpp"

namespace libp2p {
  namespace transport {
    /**
     * Listens to the connections from the specified addresses and reacts when
     * receiving ones
     */
    class TransportListener {
     public:
      /**
       * Switch the listener into 'listen' mode; it will react to every new
       * connection
       * @param address to listen to
       */
      virtual void listen(const multi::Multiaddress &address) = 0;

      /**
       * Get addresses, which this listener listens to
       * @return collection of those addresses
       */
      virtual std::vector<multi::Multiaddress> getAddresses() const = 0;

      /**
       * Stop listening to the connections, so that no more of them are
       * accepted on this transport
       */
      virtual void close() = 0;

      /**
       * @see close()
       * @param timeout, after which all connections on this transport are
       * destroyed, if not able to finish properly
       */
      virtual void close(uint8_t timeout) = 0;

      /**
       * Emit, when listener is initialized and ready to accept connections
       * @return signal to this event
       */
      virtual boost::signals2::signal<void()> onStartListening() const = 0;

      /**
       * Emit, when listener accepts new connection
       * @return signal to this event
       */
      virtual boost::signals2::signal<void(connection::Connection)>
      onNewConnection() const = 0;

      /**
       * Emit, when listener encounters an error
       * @return signal to this event
       */
      virtual boost::signals2::signal<void()> onError() const = 0;

      /**
       * Emit, when listener stops
       * @return signal to this event
       */
      virtual boost::signals2::signal<void()> onClose() const = 0;
    };
  }  // namespace transport
}  // namespace libp2p

#endif  // KAGOME_TRANSPORT_LISTENER_HPP
