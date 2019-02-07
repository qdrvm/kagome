/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef UGUISU_TRANSPORT_LISTENER_HPP
#define UGUISU_TRANSPORT_LISTENER_HPP

#include <vector>

#include <boost/optional.hpp>

#include "libp2p/common_objects/multiaddress.hpp"

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
      virtual void listen(const common::Multiaddress &address) = 0;

      /**
       * Get addresses, which this listener listens to
       * @return collection of those addresses
       */
      virtual std::vector<common::Multiaddress> getAddresses() const = 0;

      /**
       * Stop listening to the connections, so that no more connections are
       * accepted on this transport
       * @param timeout, after which all connections on this transport are
       * destroyed, if not able to finish properly
       */
      virtual void close(boost::optional<uint8_t> timeout = boost::none) = 0;
    };
  }  // namespace transport
}  // namespace libp2p

#endif  // UGUISU_TRANSPORT_LISTENER_HPP
