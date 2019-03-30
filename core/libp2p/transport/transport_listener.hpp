/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TRANSPORT_LISTENER_HPP
#define KAGOME_TRANSPORT_LISTENER_HPP

#include <boost/signals2/connection.hpp>  // for boost::signals2::connection
#include <functional>
#include <memory>
#include <vector>

#include "libp2p/multi/multiaddress.hpp"
#include "libp2p/transport/connection.hpp"
#include "libp2p/basic/closeable.hpp"

namespace libp2p::transport {

  /**
   * Listens to the connections from the specified addresses and reacts when
   * receiving ones
   */
 class TransportListener: public basic::Closeable {
   public:
    using NoArgsCallback = void();
    using ErrorCallback = void(const std::error_code &);
    using MultiaddrCallback = void(const multi::Multiaddress &);
    using ConnectionCallback = void(std::shared_ptr<Connection>);
    using HandlerFunc = std::function<ConnectionCallback>;

    virtual ~TransportListener() = default;

    /**
     * Switch the listener into 'listen' mode; it will react to every new
     * connection
     * @param address to listen to
     */
    virtual outcome::result<void> listen(const multi::Multiaddress &address) = 0;

    /**
     * Get addresses, which this listener listens to
     * @return collection of those addresses
     */
    virtual const std::vector<multi::Multiaddress> &getAddresses() const = 0;

    /**
     * Listener is initialized and ready to accept connections
     * @param callback to be called, when event happens
     */
    virtual boost::signals2::connection onStartListening(
        std::function<MultiaddrCallback> callback) = 0;

    /**
     * Listener accepts new connection
     * @param callback to be called, when event happens
     */
    virtual boost::signals2::connection onNewConnection(
        std::function<ConnectionCallback> callback) = 0;

    /**
     * Listener encounters an error
     * @param callback to be called, when event happens
     */
    virtual boost::signals2::connection onError(
        std::function<ErrorCallback> callback) = 0;

    /**
     * Listener stops
     * @param callback to be called, when event happens
     */
    virtual boost::signals2::connection onClose(
        std::function<NoArgsCallback> callback) = 0;
  };
}  // namespace libp2p::transport

#endif  // KAGOME_TRANSPORT_LISTENER_HPP
